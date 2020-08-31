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
#include <iomanip>
#include "config.h"
#include <fstream>
#include <tuple>

#if HAVE_OPENMP
#include <omp.h>
#endif

#include <opm/io/eclipse/EGrid.hpp>

using ParmEntry = std::tuple<std::vector<float>, std::string>;


static void printHelp() {

    std::cout << "\nThis program print cell coordinates for a selected grid cell. Egrid is input  \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-h Print help and exit.\n\n";
}



int main(int argc, char **argv) {

    int c                          = 0;
    std::string outfile            = "out";
    //std::string propfile           = "properties.grdecl";


    while ((c = getopt(argc, argv, "o:h")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        case 'o':
            outfile = optarg;
            break;
        }

    }

    int argOffset = optind;


    if ((argc - argOffset) < 4){
        std::cout << "\n! Error, input should contain name of grid file and then i1 i2 j1 j2 k1 k2 \n" << std::endl;
        exit(1);

    }

    int i1 = std::stoi(argv[argOffset+1]);
    int j1 = std::stoi(argv[argOffset+2]);
    int k1 = std::stoi(argv[argOffset+3]);

    std::string rootn = argv[argOffset];
    rootn = rootn.substr(0,rootn.size()-6);

    std::string egridfile = argv[argOffset];
    Opm::EclIO::EGrid grid1(egridfile);

    std::array<double,8> X;
    std::array<double,8> Y;
    std::array<double,8> Z;

    grid1.getCellCorners({i1-1, j1-1, k1-1}, X, Y, Z);

    for (int n = 0; n < 8; n++){
         std::cout  << " " << std::fixed << std::setprecision(4) << X[n];
         std::cout  << " " << std::fixed << std::setprecision(4) << Y[n];
         std::cout  << " " << std::fixed << std::setprecision(4) << Z[n];

         std::cout << std::endl;
    }

    return 0;
}
