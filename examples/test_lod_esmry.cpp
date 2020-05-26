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

#if HAVE_OPENMP
#include <omp.h>
#endif

#include <opm/io/eclipse/ESmry.hpp>
#include <opm/io/eclipse/EclUtil.hpp>

#include <opm/io/hdf5/Hdf5Util.hpp>

#include <opm/common/utility/FileSystem.hpp>

#include <opm/io/hdf5/H5Smry.hpp>


#include <iomanip>

static void printHelp() {

    std::cout << "\nThis program create one or more lodsmry files, designed for effective load on the demand.   \n"
              << "These files are created with input from the smspec and unsmry file. \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-f if LODSMRY file exist, this will be replaced. Default behaviour is that existing file is kept.\n"
              << "-n Maximum number of threads to be used if mulitple files should be created.\n"
              << "-h Print help and exit.\n\n";
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


    std::vector<std::string> vectorList = {"TIME", "FOPT", "WOPR:P-13P", "RPR:1", "GGOR:EFB", "GEFF:EFBINJ", "GEFF:ECFBARFT"};

    int argOffset = optind;

    auto lap0 = std::chrono::system_clock::now();

    std::vector<std::unique_ptr<Opm::Hdf5IO::H5Smry>> smry_vect;

    for (int f = argOffset; f < argc; f ++){
        smry_vect.emplace_back(new Opm::Hdf5IO::H5Smry(argv[f]));
    }


    auto lap1 = std::chrono::system_clock::now();

    for (auto key : vectorList){
        for (size_t t=0; t < smry_vect.size(); t++)
            std::vector<float> vect = smry_vect[t]->get(key);
    }

    auto lap2 = std::chrono::system_clock::now();


    std::chrono::duration<double> elapsed_seconds1 = lap1-lap0;
    std::chrono::duration<double> elapsed_seconds2 = lap2-lap1;

    std::cout << "\nruntime for opening          : " << elapsed_seconds1.count() << " seconds" << std::endl;
    std::cout << "runtime for loaing vectorlist: " << elapsed_seconds2.count() << " seconds\n" << std::endl;


    /*
    auto time = smry_vect[0]->get("TIME");
    auto fopr = smry_vect[0]->get("FOPR");

    for (auto val : fopr)
        std::cout << val << std::endl;

    */

    return 0;
}
