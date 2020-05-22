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

#include "hdf5.h"    // C lib
#include <opm/io/eclipse/Hdf5Util.hpp>


#if HAVE_OPENMP
#include <omp.h>
#endif

//#include <opm/io/eclipse/ESmry.hpp>
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

void init_h5_file(hid_t file_id, std::string name, std::vector<std::string>& keywords, std::vector<int> startd )
{
    Opm::Hdf5IO::write_str_variable(file_id, "SMSPEC", name);

    Opm::Hdf5IO::write_1d_hdf5<int>(file_id, "RSTEP",  {}, true);

    size_t nVect = keywords.size();

    std::vector<std::vector<float>> smrydata;
    smrydata.reserve(nVect);

    for (size_t n=0; n < nVect; n++)
        smrydata.push_back({});

    Opm::Hdf5IO::write_2d_hdf5<float>(file_id, "SMRYDATA", smrydata, true);
}


int main(int argc, char **argv) {

    int c                          = 0;
    int max_threads                = -1;
    bool force                     = false;

    while ((c = getopt(argc, argv, "fn:h")) != -1) {
        switch (c) {
        case 'f':
            force = true;
            break;
        case 'h':
            printHelp();
            return 0;
        case 'n':
            max_threads = atoi(optarg);
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    int argOffset = optind;

    Opm::filesystem::path inputFileName = argv[optind];

    Opm::filesystem::path h5FileName = inputFileName.parent_path() / inputFileName.stem();
    Opm::filesystem::path unsmryFileName = inputFileName.parent_path() / inputFileName.stem();
    Opm::filesystem::path smspecFileName = inputFileName.parent_path() / inputFileName.stem();

    h5FileName = h5FileName += ".H5SMRY";
    unsmryFileName = unsmryFileName += ".UNSMRY";
    smspecFileName = smspecFileName += ".SMSPEC";

    std::cout << "\nInput : " << std::endl;
    std::cout << "\n  h5FileName    : " << h5FileName << std::endl;
    std::cout << "  unsmryFileName: " << unsmryFileName << std::endl;

    if (Opm::EclIO::fileExists(h5FileName) && (true))
        remove (h5FileName);

    auto lap0 = std::chrono::system_clock::now();

    EclFile smspecfile(inputFileName);
    smspecfile.loadData();

    auto lap1 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds1 = lap1-lap0;
    std::cout << "\nruntime opening and loading smspec : " << elapsed_seconds1.count() << " seconds" << std::endl;


    auto keywords = smspecfile.get<std::string>("KEYWORDS");
    auto startd = smspecfile.get<int>("STARTDAT");

    std::string name = smspecFileName.stem().string() + std::string(".SMSPEC");

    hid_t file_id = H5Fcreate(h5FileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    init_h5_file(file_id, name, keywords, startd );

    H5Fclose(file_id);

    auto lap2 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = lap2-lap1;

    std::cout << "runtime init h5 file               : " << elapsed_seconds2.count() << " seconds" << std::endl;

    // adding time values

    EclFile unsmry_file(unsmryFileName);

    unsmry_file.loadData("PARAMS");

    auto lap3 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds3 = lap3-lap2;
    std::cout << "runtime opening and loading unsmry : " << elapsed_seconds1.count() << " seconds\n" << std::endl;

    auto arrayList = unsmry_file.getList();

    size_t tind = 0;

    double elapsed_writing = 0.0;

    // Turns automatic error printing on or off.
    herr_t status = H5Eset_auto2(NULL, NULL, NULL);

    for (size_t n = 0; n < arrayList.size(); n++){
        auto element = arrayList[n];

       // std::string arrName = std::get<0>(element);

        //if (arrName == "PARAMS"){
        if (std::get<0>(element) == "PARAMS"){

            std::vector<float> ts_data = unsmry_file.get<float>(n);

            std::cout << "adding timestep: " << std::setw(4) << tind;
            std::cout << "  time: " << std::setw(8) << std::setprecision(2) << std::fixed << ts_data[0] << " .. " << std::flush;

            auto lap00 = std::chrono::system_clock::now();

            file_id = H5Fopen(h5FileName.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);

            if ( file_id < 0) {
                size_t n = 0;
                // re-try to open 100 times, if still fails, throw exception
                while ((file_id < 0) && ( ++n < 100)){
                    std::this_thread::sleep_for(std::chrono::microseconds(5000));
                    file_id = H5Fopen(h5FileName.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
                }

                if (file_id < 0)
                    throw std::runtime_error ("not able to open file with " + std::to_string(n) + " attempts");

            }

            // 1 for is report step, 0 = is not report step (only time step)
            Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "RSTEP", 1);
            Opm::Hdf5IO::add_1d_to_2d(file_id, "SMRYDATA", ts_data);

            H5Fclose(file_id);

            auto lap01 = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_ts = lap01-lap00;

            elapsed_writing = elapsed_writing + static_cast<double>(elapsed_ts.count());

            std::cout << " + " << std::setw(8) << std::setprecision(5) << std::fixed << elapsed_ts.count();
            std::cout << " s,  total writing : " << elapsed_writing << " s" << std::endl << std::flush;

            tind++;

            //std::chrono::duration<double, std::milli> waitTime = 50;
            std::this_thread::sleep_for(std::chrono::microseconds(50000));


        }
    }


    std::cout << "\nFinished, all good \n\n";


    return 0;
}
