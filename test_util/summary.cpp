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

#include <cmath>
#include <iomanip>
#include <iostream>
#include <tuple>
#include <getopt.h>
#include <sstream>

#include <opm/io/eclipse/ESmry.hpp>
#include <opm/io/hdf5/H5Smry.hpp>


static void printHelp() {

    std::cout << "\nsummary needs a minimum of two arguments. First is smspec filename and then list of vectors  \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-h Print help and exit.\n"
              << "-l list all summary vectors.\n"
              << "-r extract data only for report steps. \n\n";
}

void printHeader(const std::vector<std::string>& keyList){

    std::cout << "--" << std::setw(14) << keyList[0];

    for (size_t n= 1; n < keyList.size(); n++){
        std::cout << std::setw(16) << keyList[n];
    }

    std::cout << std::endl;
}

std::string formatString(float data){

    std::stringstream stream;

    if (std::fabs(data) < 1e6){
       stream << std::fixed << std::setw(16) << std::setprecision(6) << data;
    } else {
       stream << std::scientific << std::setw(16) << std::setprecision(6)  << data;
    }

    return stream.str();
}

std::vector<std::string> make_smry_key_list(char **argv, int argc, int argOffset, std::unique_ptr<Opm::Hdf5IO::H5Smry>& h5smry)
{
    std::vector<std::string> smryList;

    for (int i=0; i<argc - argOffset-1; i++) {
        if (h5smry->hasKey(argv[i+argOffset+1])) {
            smryList.push_back(argv[i+argOffset+1]);
        } else {
            auto list = h5smry->keywordList(argv[i+argOffset+1]);

            for (auto vect : list)
                smryList.push_back(vect);
        }
    }

    return smryList;
}

std::vector<std::string> make_smry_key_list(char **argv, int argc, int argOffset, std::unique_ptr<Opm::EclIO::ESmry>& esmry)
{
    std::vector<std::string> smryList;

    for (int i=0; i<argc - argOffset-1; i++) {
        if (esmry->hasKey(argv[i+argOffset+1])) {
            smryList.push_back(argv[i+argOffset+1]);
        } else {
            auto list = esmry->keywordList(argv[i+argOffset+1]);

            for (auto vect : list)
                smryList.push_back(vect);
        }
    }

    return smryList;
}



int main(int argc, char **argv) {

    int c                          = 0;
    bool reportStepsOnly           = false;
    bool listKeys                  = false;
    bool use_h5smry                = false;

    std::unique_ptr<Opm::EclIO::ESmry> esmry;
    std::unique_ptr<Opm::Hdf5IO::H5Smry> h5smry;

    while ((c = getopt(argc, argv, "hrl")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        case 'r':
            reportStepsOnly=true;
            break;
        case 'l':
            listKeys=true;
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    int argOffset = optind;

    Opm::filesystem::path inputFileName = argv[argOffset];

    if (inputFileName.extension()==".H5SMRY")
        use_h5smry = true;

    if (use_h5smry)
        h5smry = std::make_unique<Opm::Hdf5IO::H5Smry>(inputFileName);
    else
        esmry = std::make_unique<Opm::EclIO::ESmry>(inputFileName);


    if (listKeys){
        std::vector<std::string> list;

        if (use_h5smry)
            list = h5smry->keywordList();
        else
            list = esmry->keywordList();

        for (size_t n = 0; n < list.size(); n++){
            std::cout << std::setw(20) << list[n];

            if (((n+1) % 5)==0){
                std::cout << std::endl;
            }
        }

        std::cout << std::endl;

        return 0;
    }

    std::vector<std::string> smryList;

    if (use_h5smry)
        smryList = make_smry_key_list(argv, argc, argOffset, h5smry);
    else
        smryList = make_smry_key_list(argv, argc, argOffset, esmry);


    if (smryList.size()==0){
        std::string message = "No summary keys specified on command line";
        std::cout << "\n!Runtime Error \n >> " << message << "\n\n";
        return EXIT_FAILURE;
    }

    std::vector<std::vector<float>> smryData;

    for (auto key : smryList) {
        std::vector<float> vect;

        if (use_h5smry)
            vect = reportStepsOnly ? h5smry->get_at_rstep(key) : h5smry->get(key);
        else
            vect = reportStepsOnly ? esmry->get_at_rstep(key) : esmry->get(key);

        smryData.push_back(vect);
    }

    printHeader(smryList);

    for (size_t s=0; s<smryData[0].size(); s++){
        for (size_t n=0; n < smryData.size(); n++){
            std::cout << formatString(smryData[n][s]);
        }
        std::cout << std::endl;
    }

    return 0;
}
