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
                  const std::vector<std::string>& units, const std::vector<int>& startd )
{

    std::vector<int> version = {0};
    Opm::Hdf5IO::write_1d_hdf5(file_id, "VERSION", version );

    Opm::Hdf5IO::write_1d_hdf5<int>(file_id, "RSTEP",  {}, true);

    Opm::Hdf5IO::write_1d_hdf5(file_id, "START_DATE", startd );

    Opm::Hdf5IO::write_1d_hdf5(file_id, "KEYS", keywords );

    Opm::Hdf5IO::write_1d_hdf5(file_id, "UNITS", units );

    size_t nVect = keywords.size();

    std::vector<std::vector<float>> smrydata;
    smrydata.reserve(nVect);

    for (size_t n=0; n < nVect; n++)
        smrydata.push_back({});

    Opm::Hdf5IO::write_2d_hdf5<float>(file_id, "SMRYDATA", smrydata, true);
}


int main(int argc, char **argv) {

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

    auto it = std::find(keywords.begin(), keywords.end(), "TIME");

    if (it == keywords.end())
        throw std::runtime_error("Time index not found");

    size_t time_index = std::distance( keywords.begin(), it);

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

    //hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    //H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
    //hid_t file_id = H5Fcreate(h5FileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);

    hid_t file_id = H5Fcreate(h5FileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);


    init_h5_file(file_id, name, keywords, units, startd);

    H5Fclose(file_id);


  /*
    herr_t testswmr = H5Fstart_swmr_write(file_id);

    if (testswmr < 0){
        std::cout << "swmr did not work !" << std::endl;
        exit(1);
    }
  */

    //H5Fclose(file_id);

    auto lap2 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = lap2-lap1;

    std::cout << "runtime init h5 file               : " << elapsed_seconds2.count() << " seconds" << std::endl;

    // adding time values

    //EclFile unsmry_file(unsmryFileName);

    //unsmry_file.loadData("PARAMS");

    auto lap3 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds3 = lap3-lap2;
    std::cout << "runtime opening and loading unsmry : " << elapsed_seconds1.count() << " seconds\n" << std::endl;

    //auto arrayList = unsmry_file.getList();

    // Turns automatic error printing on or off.
    H5Eset_auto2(NULL, NULL, NULL);

    size_t tind = 0;
    double elapsed_writing = 0.0;

    for (size_t n = 0; n < tsData.size(); n++) {

        std::vector<float> ts_vector = tsData[n];

        std::cout << "adding step: " << std::setw(4) << tind+1;
        std::cout << "/ " << std::setw(4) << nTstep;
        std::cout << "  time: " << std::setw(8) << std::setprecision(2) << std::fixed << ts_vector[time_index];
        std::cout << " .. " << std::flush;

        auto lap00 = std::chrono::system_clock::now();

        hid_t file_id = H5Fopen( h5FileName.c_str(), H5F_ACC_RDWR, H5P_DEFAULT );

        if (file_id < 0){
            size_t max_reopen = 100;
            size_t r = 0;

            while ((file_id < 0) && (r < max_reopen)){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                r++;
                file_id = H5Fopen( h5FileName.c_str(), H5F_ACC_RDWR, H5P_DEFAULT );
            }

            if (file_id < 0){
                std::cout << "\n!Error\n";
                throw std::runtime_error("not able to open file after " + std::to_string(max_reopen) + " atempts");
            }
        }

        Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "RSTEP", rstep[n]);
        Opm::Hdf5IO::add_1d_to_2d_hdf5(file_id, "SMRYDATA", ts_vector);

        H5Fclose(file_id);

        auto lap01 = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_ts = lap01-lap00;

        elapsed_writing = elapsed_writing + static_cast<double>(elapsed_ts.count());

        std::cout << " + " << std::setw(8) << std::setprecision(5) << std::fixed << elapsed_ts.count();
        std::cout << " s,  total writing : " << elapsed_writing << " s" << std::endl << std::flush;

        tind++;

        //std::this_thread::sleep_for(std::chrono::microseconds(50000));
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    //H5Fclose(file_id);

    std::cout << "\nFinished, all good \n\n";


/*
    std::cout << "H5_VERS_MAJOR     :  " << H5_VERS_MAJOR << std::endl;
    std::cout << "H5_VERS_MINOR     :  " << H5_VERS_MINOR << std::endl;
    std::cout << "H5_VERS_RELEASE   :  " << H5_VERS_RELEASE << std::endl;
    std::cout << "H5_VERS_SUBRELEASE:  " << H5_VERS_SUBRELEASE << std::endl;
    std::cout << "H5_VERS_INFO      :  " << H5_VERS_INFO << std::endl;
*/

    return 0;
}
