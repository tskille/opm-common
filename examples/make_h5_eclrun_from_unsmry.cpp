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

#include "hdf5.h"    // C lib
#include <opm/io/eclipse/Hdf5Util.hpp>


#if HAVE_OPENMP
#include <omp.h>
#endif

//#include <opm/io/eclipse/ESmry.hpp>
#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EclUtil.hpp>
#include <opm/common/utility/FileSystem.hpp>

using namespace Opm::EclIO;
using EclEntry = EclFile::EclEntry;


static void printHelp() {

    std::cout << "\nxxxxxx.   \n"
              << "xxxxxx. \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-x xxxxxx.\n"
              << "-h Print help and exit.\n\n";
}

std::vector<std::string> init_h5_file(hid_t file_id, std::string name, std::vector<std::string>& keywords, std::vector<int> startd )
{

    std::vector<std::string> dset_name_list;
    dset_name_list.reserve(keywords.size());

    startd.push_back(0);

    hid_t group_id = H5Gcreate2(file_id, "/general", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    //std::cout << "writing name: '" << name << "'" << std::endl;

    std::vector<int> checksum = {633668666};
    std::vector<int> version = {1, 7};

    //Opm::Hdf5IO::write_1d_hdf5(file_id, "/general/checksum",  checksum);

    Opm::Hdf5IO::write_str_variable(file_id, "/general/name", name);

    Opm::Hdf5IO::write_1d_hdf5(file_id, "/general/start_date",  startd);
    Opm::Hdf5IO::write_1d_hdf5<float>(file_id, "/general/time",  {}, true);
    Opm::Hdf5IO::write_1d_hdf5<int>(file_id, "/general/version",  version);

    group_id = H5Gcreate2(file_id, "/summary_vectors", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    std::set<std::string> sub_groups;

    for (size_t n = 0; n < keywords.size(); n++) {

        std::string keyw = keywords[n];

        std::string group_name = std::string("/summary_vectors/") + keyw;

        if (sub_groups.find(keyw) == sub_groups.end()) {
            hid_t group_id_summary = H5Gcreate2(file_id, group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            sub_groups.insert(keyw);
        }

        group_name = group_name + std::string("/") + std::to_string(n);
        std::string array_name = group_name + std::string("/values");

        group_id = H5Gcreate2(file_id, group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        Opm::Hdf5IO::write_1d_hdf5<float>(file_id, array_name,  {}, true);

        dset_name_list.push_back(array_name);

    }

    return dset_name_list;
}


void add_timestep(hid_t file_id, std::vector<float>& ts_data, std::vector<std::string> dset_name_list)
{

    Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "/general/time", ts_data[0]);

    for (size_t n = 0; n < dset_name_list.size() ; n++)
        Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, dset_name_list[n].c_str(), ts_data[n]);
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

    h5FileName = h5FileName += ".h5";
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

    auto dset_name_list = init_h5_file(file_id, name, keywords, startd );

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
            add_timestep(file_id, ts_data, dset_name_list);
            H5Fclose(file_id);

            auto lap01 = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_ts = lap01-lap00;

            elapsed_writing = elapsed_writing + static_cast<double>(elapsed_ts.count());

            std::cout << " + " << elapsed_ts.count() << " s,  total writing : " << elapsed_writing << " s" << std::endl << std::flush;

            tind++;
        }
    }


    std::cout << "\nFinished, all good \n\n";


    return 0;
}
