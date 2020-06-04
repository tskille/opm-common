/*
  Copyright 2019 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <iostream>
#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <set>
#include <iomanip>

#include <cstdlib>

#include <math.h>

#include "hdf5.h"    // C lib
#include <opm/io/hdf5/Hdf5Util.hpp>


#if HAVE_OPENMP
#include <omp.h>
#endif

#include <opm/io/eclipse/ESmry.hpp>
#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclUtil.hpp>
#include <opm/common/utility/FileSystem.hpp>

#include <chrono> // std::chrono::microseconds
#include <thread> // std::this_thread::sleep_for

using namespace Opm::EclIO;
using EclEntry = EclFile::EclEntry;

static void printHelp() {

    std::cout << "\nxxxxxx.   \n"
              << "xxxxxx. \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-x xxxxxx.\n"
              << "-h Print help and exit.\n\n";
}

void init_h5_file(hid_t file_id, std::string name, const std::vector<std::string>& keywords,
                  const std::vector<std::string>& units, const std::vector<int>& startd, size_t nTstep )
{

    std::vector<int> version = {0};
    Opm::Hdf5IO::write_1d_hdf5(file_id, "VERSION", version );

    std::vector<int> dummy_rstep;
    dummy_rstep.resize(nTstep, -1);

    Opm::Hdf5IO::write_1d_hdf5<int>(file_id, "RSTEP",  dummy_rstep);

    //Opm::Hdf5IO::expand_1d_dset<int>(file_id, "RSTEP", 3, -2);

    size_t nVect = keywords.size();

    std::vector<std::vector<float>> smrydata;
    smrydata.reserve(nVect);

    for (size_t n=0; n < nVect; n++){
        std::vector<float> emptyVect;
        emptyVect.resize(nTstep, nanf(""));
        smrydata.push_back(emptyVect);
    }

    Opm::Hdf5IO::write_2d_hdf5<float>(file_id, "SMRYDATA", smrydata);

    Opm::Hdf5IO::write_1d_hdf5(file_id, "START_DATE", startd );

    Opm::Hdf5IO::write_1d_hdf5(file_id, "KEYS", keywords );

    Opm::Hdf5IO::write_1d_hdf5(file_id, "UNITS", units );

}

void rewrie_h5_file(hid_t& file_id, size_t& maxTstep, std::string fileName, size_t inc_factor)
{
    std::cout << "\n > exceeded maximum number of time steps, start rewrite .."  << std::flush;

    auto lap0 = std::chrono::system_clock::now();


    H5Fclose(file_id);

    file_id = H5Fopen( fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    auto startd = Opm::Hdf5IO::get_1d_hdf5<int>(file_id, "START_DATE");
    auto version = Opm::Hdf5IO::get_1d_hdf5<int>(file_id, "VERSION");
    auto keys = Opm::Hdf5IO::get_1d_hdf5<std::string>(file_id, "KEYS");
    auto units = Opm::Hdf5IO::get_1d_hdf5<std::string>(file_id, "UNITS");

    auto rstep = Opm::Hdf5IO::get_1d_hdf5<int>(file_id, "RSTEP");
    auto smrydata = Opm::Hdf5IO::get_2d_hdf5<float>(file_id, "SMRYDATA");

    H5Fclose(file_id);

    size_t nTstep = maxTstep;


    std::vector<int> new_chunk;
    maxTstep = nTstep * inc_factor;

    new_chunk.resize(maxTstep - nTstep, -1);
    rstep.insert(rstep.end(), new_chunk.begin(), new_chunk.end());

    std::vector<float> new_vect_chunk;
    new_vect_chunk.resize(maxTstep - nTstep, nanf(""));

    for (size_t n=0; n < smrydata.size(); n++)
        smrydata[n].insert(smrydata[n].end(), new_vect_chunk.begin(), new_vect_chunk.end());

    if (Opm::EclIO::fileExists(fileName))
        remove (fileName.c_str());

    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

    file_id = H5Fcreate(fileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);

    Opm::Hdf5IO::write_1d_hdf5(file_id, "VERSION", version );
    Opm::Hdf5IO::write_1d_hdf5<int>(file_id, "RSTEP",  rstep);
    Opm::Hdf5IO::write_2d_hdf5<float>(file_id, "SMRYDATA", smrydata);
    Opm::Hdf5IO::write_1d_hdf5(file_id, "START_DATE", startd );
    Opm::Hdf5IO::write_1d_hdf5(file_id, "KEYS", keys );
    Opm::Hdf5IO::write_1d_hdf5(file_id, "UNITS", units );

    herr_t testswmr = H5Fstart_swmr_write(file_id);

    if (testswmr < 0){
        std::cout << "swmr did not work !" << std::endl;
        exit(1);
    }

    auto lap1 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_rewrite = lap1-lap0;

    std::cout << " + " << std::setw(8) << std::setprecision(5) << std::fixed << elapsed_rewrite.count();
    std::cout << "\n" << std::endl << std::flush;
}



/*
    std::cout << "H5_VERS_MAJOR     :  " << H5_VERS_MAJOR << std::endl;
    std::cout << "H5_VERS_MINOR     :  " << H5_VERS_MINOR << std::endl;
    std::cout << "H5_VERS_RELEASE   :  " << H5_VERS_RELEASE << std::endl;
    std::cout << "H5_VERS_SUBRELEASE:  " << H5_VERS_SUBRELEASE << std::endl;
    std::cout << "H5_VERS_INFO      :  " << H5_VERS_INFO << std::endl;
*/


int main(int argc, char **argv) {

    std::cout << "\n!testing version \n" << std::endl;

    int c                          = 0;

    while ((c = getopt(argc, argv, "h")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        default:
            return EXIT_FAILURE;
        }
    }

    int argOffset = optind;

    Opm::filesystem::path inputFileName = argv[argOffset];


    Opm::filesystem::path h5FileName = inputFileName.parent_path() / inputFileName.stem();
    Opm::filesystem::path unsmryFileName = inputFileName.parent_path() / inputFileName.stem();
    Opm::filesystem::path smspecFileName = inputFileName.parent_path() / inputFileName.stem();

    h5FileName = h5FileName += ".H5SMRY";
    //h5FileName = h5FileName += ".h5";
    unsmryFileName = unsmryFileName += ".UNSMRY";
    smspecFileName = smspecFileName += ".SMSPEC";

    std::cout << "\nInput : " << std::endl;
    std::cout << "\n  h5FileName    : " << h5FileName << std::endl;
    std::cout << "  unsmryFileName: " << unsmryFileName << std::endl;

    if (Opm::EclIO::fileExists(h5FileName) && (true))
        remove (h5FileName);

    ESmry smry1(inputFileName);

    smry1.LoadData();

    std::vector<std::vector<float>> vectData;
    std::vector<std::vector<float>> tsData;

    std::vector<std::string> keywords = smry1.keywordList();

    int time_ind = -1;

    {
        auto it = std::find(keywords.begin(), keywords.end(), "TIME");
        time_ind = std::distance(keywords.begin(), it);
    }

    std::vector<std::string> units;
    units.reserve(keywords.size());

    for (auto key : keywords)
        units.push_back(smry1.get_unit(key));

    size_t nVect = keywords.size();
    size_t nTstep = smry1.numberOfTimeSteps();

    vectData.resize(nVect, {});

    for (size_t n = 0; n < keywords.size(); n++){
        vectData[n] = smry1.get(keywords[n]);
    }

    std::cout << "\nnVect : " << nVect << std::endl;
    std::cout << "nTstep: " << nTstep << std::endl;

    auto timev = smry1.get("TIME");
    auto timevr = smry1.get_at_rstep("TIME");

    std::vector<int> rstep;
    rstep.reserve(nTstep);

    for (size_t n = 0; n < nTstep; n++){

        auto it = std::find(timevr.begin(), timevr.end(), timev[n]);

        if (it == timevr.end())
            rstep.push_back(0);
        else
            rstep.push_back(1);
    }

    tsData.resize(nTstep, {});

    for (size_t n = 0; n < nTstep; n++){
        tsData[n].reserve(nVect);
        for (size_t v = 0; v < nVect; v++){
            tsData[n].push_back(vectData[v][n]);
        }
    }

    auto lap0 = std::chrono::system_clock::now();

    EclFile smspecfile(inputFileName);
    smspecfile.loadData();

    auto lap1 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds1 = lap1-lap0;
    std::cout << "\nruntime opening and loading smspec : " << elapsed_seconds1.count() << " seconds" << std::endl;

    auto startd = smspecfile.get<int>("STARTDAT");

    std::string name = smspecFileName.stem().string() + std::string(".SMSPEC");

    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

    hid_t file_id = H5Fcreate(h5FileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);

    size_t maxTstep = 500;
    //size_t maxTstep = 5;

    init_h5_file(file_id, name, keywords, units, startd, maxTstep);

    herr_t testswmr = H5Fstart_swmr_write(file_id);

    if (testswmr < 0){
        std::cout << "swmr did not work !" << std::endl;
        exit(1);
    }

    auto lap2 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = lap2-lap1;

    std::cout << "runtime init h5 file               : " << elapsed_seconds2.count() << " seconds" << std::endl;

    // adding time values

    EclFile unsmry_file(unsmryFileName);

    unsmry_file.loadData("PARAMS");

    auto lap3 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds3 = lap3-lap2;
    std::cout << "runtime opening and loading unsmry : " << elapsed_seconds1.count() << " seconds\n" << std::endl;

    //auto arrayList = unsmry_file.getList();

    size_t tind = 0;

    double elapsed_writing = 0.0;

    //size_t time_ind = 10455;

    //for (size_t n = 0; n < 600; n++) {
    for (size_t n = 0; n < tsData.size(); n++) {
    //for (size_t n = 0; n < maxTstep-100; n++) {

        std::vector<float> ts_vector = tsData[n];

        if (n >= maxTstep) {

            rewrie_h5_file(file_id, maxTstep, h5FileName, 2);

            //std::cout << "\nneed to increase size, from " << maxTstep << std::flush;
            //std::cout << " to: " << maxTstep*2 << "\n" << std::endl << std::flush;;

            //Opm::Hdf5IO::expand_1d_dset<int>(file_id, "RSTEP", 2, -1);
            //Opm::Hdf5IO::expand_2d_dset<float>(file_id, "SMRYDATA", 2, nanf(""));

            //maxTstep = maxTstep * 2;
        }


        std::cout << "adding step: " << std::setw(4) << tind+1;
        std::cout << "/ " << std::setw(4) << nTstep;
        std::cout << "  time: " << std::setw(8) << std::setprecision(2) << std::fixed << ts_vector[time_ind] << " .. " << std::flush;

        auto lap00 = std::chrono::system_clock::now();


        Opm::Hdf5IO::set_value_for_1d_hdf5<int>(file_id, "RSTEP", n, 1);
        Opm::Hdf5IO::set_value_for_2d_hdf5<float>(file_id, "SMRYDATA", n, ts_vector);

        // 1 for is report step, 0 = is not report step (only time step)
        //Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "RSTEP", rstep[n]);
        //Opm::Hdf5IO::add_1d_to_2d_hdf5(file_id, "SMRYDATA", ts_vector);

        auto lap01 = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_ts = lap01-lap00;

        elapsed_writing = elapsed_writing + static_cast<double>(elapsed_ts.count());

        std::cout << " + " << std::setw(8) << std::setprecision(5) << std::fixed << elapsed_ts.count();
        std::cout << " s,  total writing : " << elapsed_writing << " s" << std::endl << std::flush;

        tind++;

        //std::this_thread::sleep_for(std::chrono::microseconds(50000));
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    H5Fclose(file_id);

    auto lap4 = std::chrono::system_clock::now();
    std::chrono::duration<double> final_elapsed_tot = lap4-lap3;

    std::cout << "\nFinished, total elapsed: " << final_elapsed_tot.count() << "\n\n";


    return 0;
}
