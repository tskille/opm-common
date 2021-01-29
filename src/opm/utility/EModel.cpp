/*
   Copyright 2019 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <opm/utility/EModel.hpp>
#include <opm/common/utility/FileSystem.hpp>

#include <opm/common/utility/numeric/calculateCellVol.hpp>
#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>


using EclEntry = std::tuple<std::string, Opm::EclIO::eclArrType, long int>;
using ParamEntry = std::tuple<std::string, Opm::EclIO::eclArrType>;


EModel::EModel(const std::string& filename)
{
    hasEgrid=false;

    std::string rootN;

    if (filename.substr(filename.size()-5,5)==".INIT") {
        rootN=filename.substr(0,filename.size()-5);
    } else {
        rootN=filename;
    }

    if ( Opm::filesystem::exists(rootN + ".EGRID") ){
        hasEgrid = true;
        gridfile = std::make_unique<Opm::EclIO::EGrid>(rootN+".EGRID");
    }

    initfile = std::make_unique<Opm::EclIO::EclFile>(rootN+".INIT");

    if (!initfile->hasKey("INTEHEAD")) {
        std::string message="parameter INTEHEAD not found in selected init file " + rootN+".INIT";
        throw std::invalid_argument(message);
    }

    std::vector<int> inteh=initfile->get<int>("INTEHEAD");

    nI = inteh[8];
    nJ = inteh[9];
    nK = inteh[10];

    nActive = static_cast<unsigned int>(inteh[11]);

    if (!initfile->hasKey("PORV")) {
        std::string message="parameter PORV not found in selected init file " + rootN+".INIT";
        throw std::invalid_argument(message);
    }

    PORV.reserve(nActive);

    I.reserve(nActive);
    J.reserve(nActive);
    K.reserve(nActive);

    ActFilter.resize(nActive, true);

    std::vector<float> porv_all = initfile->get<float>("PORV");

    int n = 0;

    for (int k = 0; k < nK; k++)
        for (int j = 0; j < nJ; j++)
            for (int i = 0; i < nI; i++) {
                if (porv_all[n] > 0.0) {
                    PORV.push_back(porv_all[n]);
                    I.push_back(i+1);
                    J.push_back(j+1);
                    K.push_back(k+1);
                }
                n++;
            }

    int index = 0;

    initParam["PORV"] = index;
    initParamName.push_back("PORV");
    initParamType.push_back(Opm::EclIO::REAL);
    indInInitEclfile.push_back(-1);
    index++;

    if (hasEgrid == true){
        initParam["CELLVOL"] = index;
        initParamName.push_back("CELLVOL");
        initParamType.push_back(Opm::EclIO::REAL);
        indInInitEclfile.push_back(-1);
        index++;
    }

    auto initArrList = initfile->getList();

    for (size_t m = 0; m < initArrList.size(); m++) {
        std::string name = std::get<0>(initArrList[m]);
        auto arrType = std::get<1>(initArrList[m]);
        auto sizeArray=std::get<2>(initArrList[m]);

        if (static_cast<size_t>(sizeArray) == nActive) {
            initParam[name] = index;
            initParamName.push_back(name);
            initParamType.push_back(arrType);
            indInInitEclfile.push_back(m);
            index++;
        }
    }

    if ( Opm::filesystem::exists( rootN + ".UNRST" ) )
    {
        rstfile = std::make_unique<Opm::EclIO::ERst>(rootN + ".UNRST");
        std::vector<int> rstepList = rstfile->listOfReportStepNumbers();

        if (rstepList.size() == 0)
            throw std::runtime_error("selected restart file have no report steps");

        activeReportStep = rstepList[0];
        initSolutionData(activeReportStep);

    } else {
        activeReportStep = -1;
    }

    celVolCalculated = false;
    activeFilter = false;
}

void EModel::setReportStep(int rstep)
{
    if (activeReportStep == -1)
        throw std::runtime_error("Not able to set report step since restart file not found");

    initSolutionData(rstep);
}

void EModel::initSolutionData(int rstep){

    if (!hasReportStep(rstep))
        throw std::runtime_error("restart file not found");

    activeReportStep = rstep;

    solutionParam.clear();
    solutionParamName.clear();
    solutionParamType.clear();
    indInRstEclfile.clear();

    auto rstArrList=rstfile->listOfRstArrays(activeReportStep);

    bool solparam = false;

    int index = -1;

    for (size_t n = 0; n < rstArrList.size(); n++) {
        std::string name = std::get<0>(rstArrList[n]);
        auto arrType = std::get<1>(rstArrList[n]);
        auto sizeArray=std::get<2>(rstArrList[n]);

        if (name == "ENDSOL")
            solparam=false;

        if ((solparam == true) && (static_cast<size_t>(sizeArray) == nActive)) {
            index++;
            solutionParam[name] = index;
            solutionParamName.push_back(name);
            solutionParamType.push_back(arrType);
            indInRstEclfile.push_back(n);
        }

        if (name == "STARTSOL")
            solparam = true;
    }
}

void EModel::calcCellVol()
{
    if (!hasEgrid){
        std::string message = "\nnot possible to calculate cell columes without an Egrid file. ";
        message = message + "The grid file must have same root name as the init file selected for this object";
        throw std::runtime_error(message);
    }

    CELLVOL.clear();

    int i,j,k;

    std::array<double,8> X1 = { 0.0 };
    std::array<double,8> Y1 = { 0.0 };
    std::array<double,8> Z1 = { 0.0 };

    for (size_t n = 0;n < nActive; n++){
        i=I[n]-1;  j=J[n]-1;  k=K[n]-1;
        gridfile->getCellCorners({i,j,k}, X1, Y1, Z1);
        CELLVOL.push_back(static_cast<float>(calculateCellVol(X1, Y1, Z1)));
    }

    celVolCalculated = true;
}

std::vector<ParamEntry> EModel::getListOfParameters() const
{
    std::vector<ParamEntry> res;

    for (size_t i = 0; i < initParamName.size(); i++) {
	    ParamEntry entry = std::make_tuple(initParamName[i],initParamType[i]);
        res.push_back(entry);
    }

    for (size_t i = 0; i<solutionParamName.size(); i++) {
        ParamEntry entry = std::make_tuple(solutionParamName[i],solutionParamType[i]);
        res.push_back(entry);
    }

    return res;
}

int EModel::getNumberOfActiveCells()
{
    return std::count(ActFilter.begin(), ActFilter.end(), true);
}

bool EModel::hasInitParameter(const std::string &name) const
{
    auto search = initParam.find(name);
    return search != initParam.end();
}

bool EModel::hasSolutionParameter(const std::string &name) const
{
    auto search = solutionParam.find(name);
    return search != solutionParam.end();
}

bool EModel::hasParameter(const std::string &name) const
{
    return hasInitParameter(name) || hasSolutionParameter(name);
}


bool EModel::hasReportStep(int rstep)
{
    if (activeReportStep == -1)
        return false;
    else
        return rstfile->hasReportStepNumber(rstep);
}


void EModel::resetFilter()
{
    activeFilter=false;
    std::fill(ActFilter.begin(), ActFilter.end(), true);
}


template <typename T2>
void EModel::updateActiveFilter(const std::vector<T2>& paramVect, const std::string opperator, T2 value)
{
    if (opperator == "eq") {
       for (size_t i = 0; i < paramVect.size(); i++)
            if ((ActFilter[i]) && (paramVect[i] != value))
                ActFilter[i] = false;

    } else if (opperator=="lt") {
        for (size_t i = 0; i < paramVect.size(); i++)
            if ((ActFilter[i]) && (paramVect[i] >= value))
                ActFilter[i] = false;

    } else if (opperator == "gt") {
        for (size_t i = 0; i < paramVect.size(); i++)
            if ((ActFilter[i]) && (paramVect[i] <= value))
                ActFilter[i] = false;

    } else {
        std::string message = "Unknown opprator " + opperator + ", used to set filter";
        throw std::invalid_argument(message);
    }

    activeFilter = true;
}

template <typename T2>
void EModel::updateActiveFilter(const std::vector<T2>& paramVect, const std::string opperator, T2 value1, T2 value2)
{
    if (opperator == "between") {
        for (size_t i = 0; i < paramVect.size(); i++)
            if ((ActFilter[i]) && ((paramVect[i] <= value1) || (paramVect[i] >= value2)))
                ActFilter[i] = false;

    } else {
        throw std::invalid_argument("Unknown opprator " + opperator + ", used to set filter");
    }

    activeFilter = true;
}

template <typename T2>
const std::vector<T2>& EModel::get_filter_param(std::string param)
{
    if constexpr (std::is_same<T2, int>::value){
        if ((param == "I") || (param == "ROW"))
            return  I;
        else if ((param == "J") || (param == "COLUMN"))
            return  J;
        else if ((param == "K") || (param == "LAYER"))
            return  K;
        else if (hasInitParameter(param))
            return initfile->get<int>(param);

        throw std::invalid_argument("parameter " + param + ", used to set filter, could not be found");

    } else if constexpr (std::is_same<T2, float>::value){
        if (param == "PORV")
            return  PORV;
        else if (param == "CELLVOL"){
            if (!celVolCalculated)
                calcCellVol();
            return CELLVOL;
        } else if (hasInitParameter(param))
            return initfile->get<float>(param);

        else if (hasSolutionParameter(param))
            return getSolutionFloat(param);

        throw std::invalid_argument("parameter " + param + ", used to set filter, could not be found");
    }
}


template <>
void EModel::addFilter<int>(std::string param1, std::string opperator, int num)
{
    std::vector<int> paramVect = get_filter_param<int>(param1);
    updateActiveFilter(paramVect, opperator, num);
}

template <>
void EModel::addFilter<int>(std::string param1, std::string opperator, int num1, int num2)
{
    std::vector<int> paramVect = get_filter_param<int>(param1);;
    updateActiveFilter(paramVect, opperator, num1, num2);
}

template <>
void EModel::addFilter<float>(std::string param1, std::string opperator, float num)
{
    std::vector<float> paramVect = get_filter_param<float>(param1);
    updateActiveFilter(paramVect, opperator, num);
}


template <>
void EModel::addFilter<float>(std::string param1, std::string opperator, float num1, float num2)
{
    std::vector<float> paramVect = get_filter_param<float>(param1);
    updateActiveFilter(paramVect, opperator, num1, num2);
}


void EModel::addHCvolFilter()
{
    if (FreeWaterlevel.size()==0)
        throw std::runtime_error("free water level needs to be inputted via function setDepthfwl before using filter HC filter");

    auto eqlnum = initfile->get<int>("EQLNUM");
    auto depth = initfile->get<float>("DEPTH");
    activeFilter = true;

    for (size_t n = 0; n < eqlnum.size();n++){
        int eql = eqlnum[n];
        float fwl = FreeWaterlevel[eql-1];

        if ((ActFilter[n]) && (depth[n] > fwl))
            ActFilter[n] = false;
    }
}


template <>
const std::vector<float>& EModel::getParam<float>(const std::string& name)
{
    if (activeFilter) {
        std::vector<float> param = get_filter_param<float>(name);
        filteredFloatVect.clear();

        for (size_t i = 0; i < param.size(); i++)
            if (ActFilter[i])
                filteredFloatVect.push_back(param[i]);

        return filteredFloatVect;

    } else {

        return get_filter_param<float>(name);
    }
}


template <>
const std::vector<int>& EModel::getParam<int>(const std::string& name)
{
    if (activeFilter) {
        std::vector<int> param = get_filter_param<int>(name);
        filteredIntVect.clear();

        for (size_t i = 0; i < param.size(); i++)
            if (ActFilter[i])
                filteredIntVect.push_back(param[i]);

        return filteredIntVect;

    } else {

        return get_filter_param<int>(name);
    }
}


const std::vector<float>& EModel::getInitFloat(const std::string& name)
{
    if (name == "PORV")
        return PORV;
    else
        return initfile->get<float>(name);
}


const std::vector<float>& EModel::getSolutionFloat(const std::string& name)
{
    if (!hasSolutionParameter(name)) {
        std::string message = "parameter " + name + " not found for step 0 in restart file ";
        throw std::invalid_argument(message);
    }

    auto search = solutionParam.find(name);
    int eclFileIndex = indInRstEclfile[search->second];

    return rstfile->getRestartData<float>(eclFileIndex, activeReportStep);
}


void EModel::setDepthfwl(const std::vector<float>& fwl)
{
    nEqlnum = fwl.size();
    FreeWaterlevel = fwl;

    std::vector<int> eqlnum = initfile->get<int>("EQLNUM");
    std::vector<int>::const_iterator it = max_element(eqlnum.begin(), eqlnum.end());
    int maxEqlnum = *it;

    if (maxEqlnum > nEqlnum){
        std::string message= "FWL not defined for all eql regions. # Contacts input: " + std::to_string(nEqlnum) + " needed (max value in EQLNUM): " + std::to_string(maxEqlnum);
        throw std::invalid_argument(message);
    }
}
