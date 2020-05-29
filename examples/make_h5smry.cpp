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
#include <opm/common/utility/FileSystem.hpp>


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

    int argOffset = optind;

#if HAVE_OPENMP
    int available_threads = omp_get_max_threads();

    if (max_threads < 0)
        max_threads = available_threads-2;
    else if (max_threads > (available_threads - 1))
        max_threads = available_threads-1;

    if (max_threads > (argc-argOffset))
        max_threads = argc-argOffset;

    omp_set_num_threads(max_threads);
#endif

    auto lap0 = std::chrono::system_clock::now();

    #pragma omp parallel for
    for (int f = argOffset; f < argc; f ++){
        Opm::filesystem::path inputFileName = argv[f];

        Opm::filesystem::path h5FileName = inputFileName.parent_path() / inputFileName.stem();
        h5FileName = h5FileName += ".H5SMRY";

        if (Opm::EclIO::fileExists(h5FileName) && (force))
            remove (h5FileName);

        Opm::EclIO::ESmry smryFile(argv[f]);
        if (!smryFile.make_h5smry_file()){
            std::cout << "\n! Warning, smspec already have one h5 file, existing kept use option -f to replace this" << std::endl;
        }
    }

    auto lap1 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds1 = lap1-lap0;
    std::cout << "\nruntime for creating " << (argc-argOffset) << " LODSMRY files: " << elapsed_seconds1.count() << " seconds\n" << std::endl;

    return 0;
}
