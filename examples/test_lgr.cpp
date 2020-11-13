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
#include <iomanip>      // std::setprecision

#include "config.h"

#if HAVE_OPENMP
#include <omp.h>
#endif

#include <opm/io/eclipse/ERft.hpp>
#include <opm/io/eclipse/EGrid.hpp>
#include <opm/io/eclipse/EInit.hpp>
#include <opm/io/eclipse/ERst.hpp>
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


void list_coord_all_lgrs(const std::string& filename)
{
    Opm::EclIO::EGrid grid1(filename);

    auto list_lgr = grid1.list_of_lgrs();

    for (auto lgr_name : list_lgr) {

        std::cout << "\nopen lgr: " << lgr_name << std::endl;

        Opm::EclIO::EGrid lgr(filename, lgr_name);

        std::array<double,8> X = {0.0};
        std::array<double,8> Y = {0.0};
        std::array<double,8> Z = {0.0};

        lgr.load_grid_data();
        lgr.getCellCorners(0, X, Y, Z);

        for (size_t n = 0; n < 8; n++)
            std::cout << X[n] << "  " << Y[n] << "  " << Z[n] << std::endl;
    }
}

void get_porv_global_grid(const std::string& filename)
{
    Opm::EclIO::EInit init1(filename);

    auto porv = init1.getInitData<float>("PORV");

    std::cout << "\nPORV main grid: \n\n";

    for (size_t n=0; n < porv.size(); n++){
        std::cout << " " << std::fixed << std::setprecision(3) << std::setw(8) << porv[n];

        if (((n+1) % 8) == 0)
            std::cout << std::endl;
    }

    std::cout << "\n\n";
}

void get_porv_all_lgrs(const std::string& grid_file_name, const std::string& init_file_name)
{

    Opm::EclIO::EGrid grid1(grid_file_name);

    auto list_lgr = grid1.list_of_lgrs();

    for (auto lgr_name : list_lgr) {

        Opm::EclIO::EInit init1(init_file_name);

        std::cout << "\nPORV : " << lgr_name << "\n\n";

        auto porv = init1.getInitData<float>("PORV", lgr_name);

        for (size_t n=0; n < porv.size(); n++) {
            std::cout << " " << std::fixed << std::setprecision(3) << std::setw(8) << porv[n];

            if (((n+1) % 8) == 0)
                std::cout << std::endl;
        }

    }

    std::cout << "\n\n";
}

void list_all_static_arrays_in_a_lgr(const std::string& init_file_name, const std::string& grid_name)
{

    Opm::EclIO::EInit init1(init_file_name);

    auto array_list = init1.list_arrays(grid_name);

    for (size_t n=0; n < array_list.size(); n++){
        std::cout << " name: " << std::setw(8) << std::get<0>(array_list[n]);
        std::cout << " type: " << std::get<1>(array_list[n]);
        std::cout << " size: " << std::setw(4) << std::get<2>(array_list[n]);
        std::cout << std::endl;
    }

    std::cout << "\n\n";
}

void list_all_static_arrays_in_the_global_grid(const std::string& init_file_name){

    Opm::EclIO::EInit init1(init_file_name);

    auto array_list = init1.list_arrays();

    for (size_t n=0; n < array_list.size(); n++){
        std::cout << " name: " << std::setw(8) << std::get<0>(array_list[n]);
        std::cout << " type: " << std::get<1>(array_list[n]);
        std::cout << " size: " << std::setw(4) << std::get<2>(array_list[n]);
        std::cout << std::endl;
    }

    std::cout << "\n\n";
}

void list_all_nncs_in_the_global_grid(const std::string& grid_file_name)
{
    Opm::EclIO::EGrid grid1(grid_file_name);

    //grid1.load_nnc_data();

    auto nnc_data = grid1.get_nnc_ijk();
    std::cout << "number of nncs " << nnc_data.size() << std::endl;

    for (auto nnc : nnc_data){
        std::cout << std::get<0>(nnc) << " " << std::get<1>(nnc) << " " << std::get<2>(nnc) << " -> ";
        std::cout << std::get<3>(nnc) << " " << std::get<4>(nnc) << " " << std::get<5>(nnc) << "  ";
        std::cout << " trans: " << std::setw(8) << std::setprecision(5) << std::get<6>(nnc) << std::endl;
    }

    std::cout << "\n\n";
}

void list_all_nncs_in_a_lgr(const std::string& grid_file_name, const std::string& grid_name)
{
    Opm::EclIO::EGrid lgr(grid_file_name, grid_name);

    lgr.load_nnc_data();

    auto nnc_data = lgr.get_nnc_ijk();

    std::cout << "number of nncs " << nnc_data.size() << std::endl;

    for (auto nnc : nnc_data){
        std::cout << std::get<0>(nnc) << " " << std::get<1>(nnc) << " " << std::get<2>(nnc) << " -> ";
        std::cout << std::get<3>(nnc) << " " << std::get<4>(nnc) << " " << std::get<5>(nnc) << "  ";
        std::cout << " trans: " << std::setw(8) << std::setprecision(5) << std::get<6>(nnc) << std::endl;
    }

    std::cout << "\n\n";
}

void list_all_dynamic_arrays_in_the_global_grid_1(const std::string& rst_file_name)
{
    Opm::EclIO::ERst rst1(rst_file_name);

    // this will list arrays also in lgrs
    auto array_list = rst1.listOfRstArrays(2);

    for (size_t n=0; n < array_list.size(); n++){
        std::cout << " name: " << std::setw(8) << std::get<0>(array_list[n]);
        std::cout << " type: " << std::get<1>(array_list[n]);
        std::cout << " size: " << std::setw(4) << std::get<2>(array_list[n]);
        std::cout << std::endl;
    }

    std::cout << "\n\n";
}

void list_all_dynamic_arrays_in_the_global_grid_2(const std::string& rst_file_name)
{
    Opm::EclIO::ERst rst1(rst_file_name);

    // this will list arrays also in lgrs
    auto array_list = rst1.listOfRstArrays(2, "global");

    // this will do the same
    //auto array_list = rst1.listOfRstArrays(2, "");

    for (size_t n=0; n < array_list.size(); n++){
        std::cout << " name: " << std::setw(8) << std::get<0>(array_list[n]);
        std::cout << " type: " << std::get<1>(array_list[n]);
        std::cout << " size: " << std::setw(4) << std::get<2>(array_list[n]);
        std::cout << std::endl;
    }

    std::cout << "\n\n";
}


void list_all_dynamic_arrays_in_a_lgr(const std::string& rst_file_name, const std::string& grid_name)
{
    Opm::EclIO::ERst rst1(rst_file_name);

    auto array_list = rst1.listOfRstArrays(2, grid_name);

    for (size_t n=0; n < array_list.size(); n++){
        std::cout << " name: " << std::setw(8) << std::get<0>(array_list[n]);
        std::cout << " type: " << std::get<1>(array_list[n]);
        std::cout << " size: " << std::setw(4) << std::get<2>(array_list[n]);
        std::cout << std::endl;
    }

    std::cout << "\n\n";
}

void get_pres_in_the_global_grid_rstep_1(const std::string& rst_file_name, const std::string& param, int rstep)
{
    Opm::EclIO::ERst rst1(rst_file_name);

    auto array = rst1.getRestartData<float>(param, rstep);

    std::cout << "\nparameter : " << param << "  report step " << rstep;
    std::cout << "  size: " << array.size() << "\n\n";

    for (size_t n=0; n < array.size(); n++) {
        std::cout << " " << std::fixed << std::setprecision(3) << std::setw(8) << array[n];

        if (((n+1) % 8) == 0)
            std::cout << std::endl;
    }

    std::cout << "\n\n";
}

void get_pres_in_in_a_lgr_rstep_1(const std::string& rst_file_name, const std::string& param,
                                  int rstep,  const std::string& grid_name)
{
    Opm::EclIO::ERst rst1(rst_file_name);

    auto array = rst1.getRestartData<float>(param, rstep, grid_name);

    std::cout << "\nGrid: " << grid_name << ", parameter : " << param << "  report step " << rstep;
    std::cout << "  size: " << array.size() << "\n\n";

    for (size_t n=0; n < array.size(); n++) {
        std::cout << " " << std::fixed << std::setprecision(3) << std::setw(8) << array[n];

        if (((n+1) % 8) == 0)
            std::cout << std::endl;
    }

    std::cout << "\n\n";
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

    Opm::filesystem::path inputGridFileName = argv[argOffset];

    std::string ext(inputGridFileName.extension());
    bool formatted = ext.substr(1,1) =="F" ? true : false;

    Opm::filesystem::path rootName = inputGridFileName.parent_path() / inputGridFileName.stem();
    Opm::filesystem::path initFileName = rootName;
    Opm::filesystem::path rstFileName = rootName;

    if (formatted){
        rstFileName += ".FUNRST";
        initFileName += ".FINIT";
    } else {
        rstFileName += ".UNRST";
        initFileName += ".INIT";
    }

    std::cout << "filename: " << inputGridFileName << std::endl;



    // example functions showing usage

    //list_coord_all_lgrs(inputGridFileName);

    // get_porv_global_grid(initFileName);
    // get_porv_all_lgrs(inputGridFileName, initFileName);
    // list_all_static_arrays_in_the_global_grid(initFileName);
    // list_all_static_arrays_in_a_lgr(initFileName, "LGR2");
    // list_all_nncs_in_the_global_grid(inputGridFileName);
    // list_all_nncs_in_a_lgr(inputGridFileName, "LGR2");

    // list_all_dynamic_arrays_in_the_global_grid_1(rstFileName);
    // list_all_dynamic_arrays_in_the_global_grid_2(rstFileName);
    // list_all_dynamic_arrays_in_a_lgr(rstFileName, "LGR2");

    //get_pres_in_the_global_grid_rstep_1(rstFileName, "PRESSURE", 1);
    //get_pres_in_in_a_lgr_rstep_1(rstFileName, "PRESSURE", 1, "LGR2");

    Opm::EclIO::EGrid grid1(inputGridFileName);

    if (grid1.is_radial())
        std::cout << "input grid is radial \n";
    else
        std::cout << "input grid cartesian \n";

    std::array<double,8> X = {0.0};
    std::array<double,8> Y = {0.0};
    std::array<double,8> Z = {0.0};

    grid1.getCellCorners({3,1,0}, X, Y, Z);

    for (size_t n=0; n< 8;n++){
        std::cout << "n=" << n << std::setw(8) << std::fixed << std::setprecision(2) << X[n];
        std::cout << std::setw(8) << std::fixed << std::setprecision(2) << Y[n];
        std::cout << std::setw(8) << std::fixed << std::setprecision(2) << Z[n];

        std::cout << std::endl;
    }

    std::cout << "finished, all good\n";
    return 0;
}
