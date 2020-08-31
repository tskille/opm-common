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
#include <sstream>

#if HAVE_OPENMP
#include <omp.h>
#endif

#include <opm/io/eclipse/EGrid.hpp>

using ParmEntry = std::tuple<std::vector<float>, std::string>;


static void printHelp() {

    std::cout << "\nThis program extract grid geometry from a full field model, and exports COORD and ZCORN keywords    \n"
              << "Input Egrid and range filter (I1 I2 J1 J2 K1 and K2) should be specified on the command line. \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-o Output root name.\n"
              << "-h Print help and exit.\n\n";
}


void write_coord(const std::vector<float>& coord, const int ncol){

    int n = 0;

    for (auto value : coord){

        n++;
        std::cout << "  " << std::fixed << std::setprecision(4) << value;

        if ((n % ncol) == 0){
            std::cout << std::endl;
        }
    }

}

void write_zcorn(const std::vector<float>& zcorn, const int ncol){

    int n = 0;

    for (auto value : zcorn){

        n++;
        std::cout << "  " << std::fixed << std::setprecision(4) << value;

        if ((n % ncol) == 0){
            std::cout << std::endl;
        }
    }

}

std::vector<float> create_coord(const Opm::EclIO::EGrid& grid1, const std::array<int, 6>& box)
{
    int nI = box[1] - box[0] + 1;
    int nJ = box[3] - box[2] + 1;

    std::vector<float> coord;
    coord.reserve((nI+1)*(nJ+1)*6);

    std::array<double,8> X;
    std::array<double,8> Y;
    std::array<double,8> Z;

    for (int j = box[2]-1; j < box[3]; j++) {

        for (int i = box[0]-1; i < box[1] ; i++) {

            grid1.getCellCorners({i,j,box[4] -1}, X, Y, Z);

            coord.push_back(X[0]);
            coord.push_back(Y[0]);
            coord.push_back(Z[0]);

            grid1.getCellCorners({i,j,box[5] -1}, X, Y, Z);

            coord.push_back(X[4]);
            coord.push_back(Y[4]);
            coord.push_back(Z[4]);
        }

        grid1.getCellCorners({box[1]-1, j, box[4] -1}, X, Y, Z);

        coord.push_back(X[1]);
        coord.push_back(Y[1]);
        coord.push_back(Z[1]);

        grid1.getCellCorners({box[1]-1,j,box[5] -1}, X, Y, Z);

        coord.push_back(X[5]);
        coord.push_back(Y[5]);
        coord.push_back(Z[5]);

    }

    for (int i = box[0] -1; i < box[1] ; i++) {

        grid1.getCellCorners({i,box[3]-1,box[4] -1}, X, Y, Z);

        coord.push_back(X[2]);
        coord.push_back(Y[2]);
        coord.push_back(Z[2]);

        grid1.getCellCorners({i,box[3]-1,box[5] -1}, X, Y, Z);

        coord.push_back(X[6]);
        coord.push_back(Y[6]);
        coord.push_back(Z[6]);
    }

    grid1.getCellCorners({box[1]-1, box[3]-1, box[4] -1}, X, Y, Z);

    coord.push_back(X[3]);
    coord.push_back(Y[3]);
    coord.push_back(Z[3]);

    grid1.getCellCorners({box[1]-1,box[3]-1,box[5] -1}, X, Y, Z);

    coord.push_back(X[7]);
    coord.push_back(Y[7]);
    coord.push_back(Z[7]);

    return coord;
}


std::vector<float> create_zcorn(const Opm::EclIO::EGrid& grid1, const std::array<int, 6>& box)
{
    int nI = box[1] - box[0] + 1;
    int nJ = box[3] - box[2] + 1;
    int nK = box[5] - box[4] + 1;

    std::vector<float> zcorn;
    zcorn.reserve( nI*nJ*nK*8);

    std::array<double,8> X;
    std::array<double,8> Y;
    std::array<double,8> Z;

    for (int k = box[4]-1; k< box[5]; k++) {

        for (int j = box[2]-1; j< box[3]; j++) {
            for (int i = box[0]-1; i< box[1]; i++) {
                grid1.getCellCorners({i, j, k}, X, Y, Z);
                zcorn.push_back(Z[0]);
                zcorn.push_back(Z[1]);
            }

            for (int i = box[0]-1; i< box[1]; i++) {
                grid1.getCellCorners({i, j, k}, X, Y, Z);
                zcorn.push_back(Z[2]);
                zcorn.push_back(Z[3]);
            }
        }

        for (int j = box[2]-1; j< box[3]; j++) {
            for (int i = box[0]-1; i< box[1]; i++) {
                grid1.getCellCorners({i, j, k}, X, Y, Z);
                zcorn.push_back(Z[4]);
                zcorn.push_back(Z[5]);
            }

            for (int i = box[0]-1; i< box[1]; i++) {
                grid1.getCellCorners({i, j, k}, X, Y, Z);
                zcorn.push_back(Z[6]);
                zcorn.push_back(Z[7]);
            }
        }

    }

    return zcorn;
}

void write_grdeclfile(const std::string filename, const std::array<int, 6>& box,
                      const std::vector<float>& coord, const std::vector<float>& zcorn)
{
    int nI = box[1] - box[0] + 1;
    int nJ = box[3] - box[2] + 1;
    int nK = box[5] - box[4] + 1;

    std::ofstream grdecl_file;
    grdecl_file.open(filename);
    grdecl_file << "SPECGRID\n";
    grdecl_file << " " << nI << " " << nJ << " " << nK << " 1 F / \n";

    grdecl_file << "\nCOORD\n";

    int n=0;
    for (auto value: coord){
        grdecl_file << " " << std::fixed << std::setprecision(4) << value;
        n++;

        if ((n % 6) == 0)
             grdecl_file << std::endl;

    }

    grdecl_file << " / \n";


    grdecl_file << "\nZCORN\n";

    n = 0;
    for (auto value: zcorn){
        grdecl_file << " " << std::fixed << std::setprecision(4) << value;
        n++;

        if ((n % 6) == 0)
             grdecl_file << std::endl;

    }

    grdecl_file << " / \n";


    grdecl_file.close();
}

std::vector<float> get_property(const Opm::EclIO::EGrid& grid1, const std::array<int, 6>& box,
                                const std::vector<float>& data)
{
    int nI = box[1] - box[0] + 1;
    int nJ = box[3] - box[2] + 1;
    int nK = box[5] - box[4] + 1;

    std::vector<float> res;
    res.reserve(nI*nJ*nK);

    for (int k = box[4]-1; k < box[5]; k++) {
        for (int j = box[2]-1; j < box[3]; j++) {
            for (int i = box[0]-1; i < box[1]; i++) {
                int actind = grid1.active_index(i, j, k);

                if (actind < 0)
                    res.push_back(0.0);
                else
                    res.push_back(data[actind]);
            }
        }
    }

    return res;
}

void write_prop_file(const std::string filename, const std::vector<ParmEntry>& paramList){

    std::ofstream prop_file;
    prop_file.open(filename);

    for (auto element : paramList) {

        std::string name = std::get<1>(element);
        prop_file << name << std::endl;
        auto data = std::get<0>(element);
        int n = 0;
        for (auto value: data) {
            prop_file << " " << std::fixed << std::setprecision(4) << value;
            n++;

            if ((n % 6) == 0)
                prop_file << std::endl;
        }

        if ((data.size() % 6) != 0)
            prop_file << std::endl;

        prop_file << " /\n" << std::endl;
    }

    prop_file.close();
}

std::vector<std::string> split(std::string str, char delimiter)
{
  std::vector<std::string> internal;
  std::stringstream ss(str); // Turn the string into a stream.
  std::string tok;

  while(std::getline(ss, tok, delimiter)) {
    internal.push_back(tok);
  }

  return internal;
}


std::vector<float> read_parameter(std::string grdeclfile, std::string name, int nI, int nJ, int nK )
{
    std::vector<float> data;
    data.reserve(nI*nJ*nK);

    std::fstream fileH;

    fileH.open(grdeclfile, std::ios::in);

    std::string line;
    bool keyFound = false;

    while (not keyFound){
        std::getline(fileH,line);

        if (line.size() >= name.size())
            if (line.substr(0,name.size()) == name)
               keyFound = true;
    }

    bool slashFound = false;

    int n=0;

    while (not slashFound){
        std::getline(fileH,line);

        if (line.find_first_of("/") != std::string::npos){
            slashFound = true;
            int pos =  line.find_first_of("/");
            line = line.substr(0, pos);
        }

        if ((line.size() > 0) && (line.substr(0,2) != "--")){
            auto tokens = split(line, ' ');

            for (auto str : tokens){

                if (str.find_first_of("*") != std::string::npos){
                    int pos =  str.find_first_of("*");

                    float value = std::stof(str.substr(pos+1));
                    int num = std::stoi(str.substr(0,pos));

                    for (int n = 0; n < num ; n++)
                        data.push_back(value);

                } else {

                    float value = std::stof(str);
                    data.push_back(value);
                }
            }
        }
        n++;
    }


    fileH.close();

    return data;

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


    if ((argc - argOffset) < 11){
        std::cout << "\n! Error, input should contain name of grid file and then i1 i2 j1 j2 k1 k2 \n" << std::endl;
        exit(1);

    }

    std::string grdeclfile = argv[argOffset];

    std::string name = argv[argOffset+1];

    int nI = std::stoi(argv[argOffset+2]);
    int nJ = std::stoi(argv[argOffset+3]);
    int nK = std::stoi(argv[argOffset+4]);

    int i1 = std::stoi(argv[argOffset+5]);
    int i2 = std::stoi(argv[argOffset+6]);

    int j1 = std::stoi(argv[argOffset+7]);
    int j2 = std::stoi(argv[argOffset+8]);

    int k1 = std::stoi(argv[argOffset+9]);
    int k2 = std::stoi(argv[argOffset+10]);


    auto data = read_parameter(grdeclfile, name, nI, nJ, nK );


    if (data.size() != ( nI*nJ*nK) ){
        std::cout << "\n!Error reading " << name << " size not correct \n";
        exit(1);
    }

    //std::cout << "should have length: " << nI*nJ*nK << std::endl;
    //std::cout << "size of data: " << data.size() << std::endl;

    for (int k = k1 -1; k < k2; k++) {
        for (int j = j1 -1; j < j2; j++) {
            for (int i = i1 -1; i < i2; i++) {
                int globInd = i + j*nI + k *nI*nJ;

                std::cout << i  << " " << j << "  " << k << "  globind: " << globInd;

                std::cout << " value: " << std::fixed << data[globInd] << std::endl;

            }
        }
    }


/*
     std::string line;
    std::getline(fileH,line);

    int p1 = line.find_first_of("'");
    int p2 = line.find_first_of("'",p1+1);
    int p3 = line.find_first_of("'",p2+1);
    int p4 = line.find_first_of("'",p3+1);



 */
    /*

    Opm::EclIO::EGrid grid1(egridfile);
    Opm::EclIO::EclFile init1(initfile);

    std::vector<ParmEntry> paramList;
    std::vector<std::string> nameList = {"PERMX", "PERMY", "PERMZ", "PORO", "NTG"};

    for (auto  name : nameList ) {
        if (init1.hasKey(name)) {
            auto data = init1.get<float>(name);
            auto data_s = get_property(grid1, {i1,i2,j1,j2,k1,k2} ,data);
            paramList.push_back(std::make_tuple(data_s, name));
        }
    }

    write_prop_file(outfile + "_props.inc", paramList);

    auto coord = create_coord(grid1, {i1,i2,j1,j2,k1,k2});
    auto zcorn = create_zcorn(grid1, {i1,i2,j1,j2,k1,k2});

    write_grdeclfile(outfile + ".grdecl", {i1,i2,j1,j2,k1,k2}, coord, zcorn);
    */

    return 0;
}
