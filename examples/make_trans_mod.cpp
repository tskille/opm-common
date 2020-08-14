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

#include <algorithm>
#include <fstream>
#include <iomanip>


#include <opm/io/eclipse/EGrid.hpp>
#include <opm/io/eclipse/EclFile.hpp>
#include <opm/common/utility/FileSystem.hpp>


static void printHelp() {

    std::cout << "\nThis program create transmissibility input (NNC and EDITNNCR) for an eclipse input  case. The program needs two input cases (root names to EGRID and INIT file).  \n\n"
              << " - The first argument is rootname for the reference case\n - The second argument is rootname for the case to be modified. \n\nWhen including the generated include files to the second case, transmissibility should be identical in the two input cases. \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-h Print help and exit.\n\n";
}


class NNC {

public:
    NNC(int i1, int j1, int k1, int i2, int j2, int k2, float trans):
      i1_(i1), j1_(j1), k1_(k1), i2_(i2), j2_(j2), k2_(k2), trans_(trans){};

void setTrans(float trans){ trans_ = trans; };

friend std::ostream& operator<< (std::ostream &out, const NNC &nnc);

friend bool operator== (const NNC &c1, const NNC &c2);
friend bool operator!= (const NNC &c1, const NNC &c2);


private:
    int i1_, j1_, k1_;
    int i2_, j2_, k2_;
    float trans_;
};


std::ostream& operator<< (std::ostream &out, const NNC &nnc)
{
    out << " " << nnc.i1_ << " " << nnc.j1_ << " " << nnc.k1_ << "   ";
    out << nnc.i2_ << " " << nnc.j2_ << " " << nnc.k2_ << "  ";
    out << std::fixed << nnc.trans_;
    return out;
}


bool operator== (const NNC &c1, const NNC &c2)
{
    bool test1 = false;
    bool test2 = false;

    if (c1.i1_== c2.i1_ && c1.j1_== c2.j1_ && c1.k1_== c2.k1_  &&
        c1.i2_== c2.i2_  && c1.j2_== c2.j2_  && c1.k2_== c2.k2_ )
        test1 = true;

    if (c1.i2_== c2.i1_ && c1.j2_== c2.j1_ && c1.k2_== c2.k1_  &&
        c1.i1_== c2.i2_  && c1.j1_== c2.j2_  && c1.k1_== c2.k2_ )
        test2 = true;

    return (test1 || test2);

}

bool operator!= (const NNC &c1, const NNC &c2)
{
    return !(c1== c2);
}


void write_keyword(const std::string fileName, const std::string name, const std::vector<float>& trans)
{
    const int ncol = 5;

    std::ofstream tran_file;
    tran_file.open(fileName);

    tran_file << "-- file holding " << name << " which should be included in EDIT section\n\n";
    tran_file << name << std::endl;

    for (size_t n=0; n < trans.size() ; n++) {
        tran_file << " " << std::fixed << trans[n];

        if (((n+1) % ncol) == 0)
            tran_file << std::endl;
    }

    if ((trans.size() % ncol) != 0)
        tran_file << std::endl;

    tran_file << "/ \n";

    tran_file.close();
}


bool verify_same_grid(const Opm::EclIO::EGrid& grid1, const Opm::EclIO::EGrid grid2)
{
    bool sameSize = true;

    auto dims1 = grid1.dimension();
    auto dims2 = grid2.dimension();

    if (dims1[0] != dims2[0] || dims1[1] != dims2[1] || dims1[2] != dims2[2])
        return false;

    int nact1 = grid1.activeCells();
    int nact2 = grid2.activeCells();

    if (nact1 != nact2)
        std::cout << "\n!Warning, not same number of active cells \n" << std::endl;

    return true;
}

int main(int argc, char **argv) {

    int c                            = 0;
    std::string output_name          = "modified_trans";

    while ((c = getopt(argc, argv, "o:h")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        case 'o':
            output_name = optarg;
            break;

        default:
            return EXIT_FAILURE;
        }
    }

    int argOffset = optind;

    if ((argc - argOffset) < 2){
        std::cout << "\nError, program needs two input arguments, use option -h for help \n" << std::endl;
        return EXIT_FAILURE;
    }

    std::string ref_rootn(argv[argOffset]);
    std::string case_rootn(argv[argOffset+1]);


    Opm::EclIO::EGrid egrid_r = Opm::EclIO::EGrid(ref_rootn + ".EGRID");
    Opm::EclIO::EGrid egrid = Opm::EclIO::EGrid(case_rootn + ".EGRID");

    if (not verify_same_grid(egrid_r, egrid)){
        std::cout << "\n!Error, grid dimensions are not same \n\n";
        exit(1);
    }

    Opm::EclIO::EclFile init_r = Opm::EclIO::EclFile(ref_rootn + ".INIT");
    Opm::EclIO::EclFile init = Opm::EclIO::EclFile(case_rootn + ".INIT");

    auto tranx_r = init_r.get<float>("TRANX");
    auto trany_r = init_r.get<float>("TRANY");
    auto tranz_r = init_r.get<float>("TRANZ");


    std::vector<NNC> nnc_list_r;

    {
        auto ncc1r = egrid_r.get<int>("NNC1");
        auto ncc2r = egrid_r.get<int>("NNC2");

        auto trans_r = init_r.get<float>("TRANNNC");

        for (size_t n = 0; n < ncc1r.size(); n++)
        {
            auto ijk_from = egrid_r.ijk_from_global_index(ncc1r[n]-1);
            auto ijk_to = egrid_r.ijk_from_global_index(ncc2r[n]-1);

            NNC nnc(ijk_from[0]+1, ijk_from[1]+1, ijk_from[2]+1, ijk_to[0]+1, ijk_to[1]+1, ijk_to[2]+1, trans_r[n]);

            nnc_list_r.push_back(nnc);
        }
    }

    std::vector<NNC> nnc_list;

    {
        auto ncc1 = egrid.get<int>("NNC1");
        auto ncc2 = egrid.get<int>("NNC2");

        auto trans = init.get<float>("TRANNNC");

        for (size_t n = 0; n < ncc1.size(); n++)
        {
            auto ijk_from = egrid.ijk_from_global_index(ncc1[n]-1);
            auto ijk_to = egrid.ijk_from_global_index(ncc2[n]-1);

            NNC nnc(ijk_from[0]+1, ijk_from[1]+1, ijk_from[2]+1, ijk_to[0]+1, ijk_to[1]+1, ijk_to[2]+1, trans[n]);

            nnc_list.push_back(nnc);
        }
    }


    std::vector<NNC> extra_nnc_list;

    for (auto nnc_r : nnc_list_r)
        if (std::find(nnc_list.begin(), nnc_list.end(), nnc_r) == nnc_list.end())
            extra_nnc_list.push_back(nnc_r);

    std::vector<NNC> remove_nnc_list;

    for (auto nnc : nnc_list)
        if (std::find(nnc_list_r.begin(), nnc_list_r.end(), nnc) == nnc_list_r.end())
            remove_nnc_list.push_back(nnc);


    //for (auto nnc : remove_nnc_list)
    //    std::cout << "remove: " << nnc << "  remove with editnnc ( * 0.0) in EDIT section " << std::endl;

    // start writing output files

    {
        std::ofstream grid_nnc_file;
        grid_nnc_file.open(output_name + "_nnc.inc");

        grid_nnc_file << "-- file holding NNC which should be included in GRID section \n\nNNC" << std::endl;

        for (auto nnc : extra_nnc_list)
            grid_nnc_file << nnc << std::endl;

        grid_nnc_file << "/ \n";

        grid_nnc_file.close();
    }

    {

        std::ofstream edit_nnc_file;
        edit_nnc_file.open(output_name + "_editnncr.inc");

        edit_nnc_file << "-- file holding EDITNNCR which should be included in EDIT section \nEDITNNCR" << std::endl;

        for (auto nnc : nnc_list_r)
            edit_nnc_file << nnc << std::endl;

        edit_nnc_file << "/ \n";

        edit_nnc_file.close();

    }

    {

        std::ofstream edit_nnc_file;
        edit_nnc_file.open(output_name + "_editnnc.inc");

        edit_nnc_file << "-- file holding EDITNNC which should be included in EDIT section (after EDITNNCR keyword)\nEDITNNC" << std::endl;

        for (auto nnc : remove_nnc_list){
            nnc.setTrans(0.0);
            edit_nnc_file << nnc << std::endl;
        }

        edit_nnc_file << "/ \n";

        edit_nnc_file.close();
    }

    write_keyword(output_name + "_tranx.inc", "TRANX", tranx_r);
    write_keyword(output_name + "_trany.inc", "TRANY", trany_r);
    write_keyword(output_name + "_tranz.inc", "TRANZ", tranz_r);

    std::cout << "\nFinished, all good \n\n";

    return 0;
}
