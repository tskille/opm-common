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

    std::vector<std::unique_ptr<Opm::Hdf5IO::H5Smry>> h5smry_vect;
    std::vector<std::unique_ptr<Opm::EclIO::ESmry>> esmry_vect;

    for (int f = argOffset; f < argc; f ++){
        std::cout << argv[f] << "\n";

        Opm::filesystem::path inputFileName = argv[f];

        if (inputFileName.extension()==".SMSPEC")
            esmry_vect.emplace_back(new Opm::EclIO::ESmry(argv[f]));
        else if (inputFileName.extension()==".H5SMRY")
            h5smry_vect.emplace_back(new Opm::Hdf5IO::H5Smry(argv[f]));
        else
            throw std::invalid_argument("only smspec and h5smry are valied input files");
    }


    auto lap1 = std::chrono::system_clock::now();

    for (auto key : vectorList)
        for (size_t t=0; t < esmry_vect.size(); t++)
            std::vector<float> vect = esmry_vect[t]->get(key);

    auto lap2 = std::chrono::system_clock::now();

    for (auto key : vectorList)
        for (size_t t=0; t < h5smry_vect.size(); t++)
            std::vector<float> vect = h5smry_vect[t]->get(key);


    auto lap3 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds1 = lap1-lap0;
    std::chrono::duration<double> elapsed_seconds2 = lap2-lap1;
    std::chrono::duration<double> elapsed_seconds3 = lap3-lap2;

    std::cout << "\nruntime for opening          : " << elapsed_seconds1.count() << " seconds" << std::endl;
    std::cout << "runtime for loaing smspec    : " << elapsed_seconds2.count() << " seconds" << std::endl;
    std::cout << "runtime for loaing h5smry    : " << elapsed_seconds3.count() << " seconds\n" << std::endl;

/*
    auto time = h5smry_vect[0]->get("TIME");
    //auto fopr = h5smry_vect[0]->get("FOPR");

    for (auto val : time)
        std::cout << val << std::endl;
*/

    return 0;
}
