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

#include <math.h>

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
                  const std::vector<std::string>& units, const std::vector<int>& startd, size_t nTstep )
{

    std::vector<int> version = {0};
    Opm::Hdf5IO::write_1d_hdf5(file_id, "VERSION", version );

    //std::vector<int> dummy_rstep;
    //dummy_rstep.resize(nTstep, -1);
    //Opm::Hdf5IO::write_1d_hdf5<int>(file_id, "RSTEP",  dummy_rstep);
    //Opm::Hdf5IO::expand_1d_dset<int>(file_id, "RSTEP", 3, -2);

    /*
    size_t nVect = keywords.size();

    std::vector<std::vector<float>> smrydata;
    smrydata.reserve(nVect);

    for (size_t n=0; n < nVect; n++){
        std::vector<float> emptyVect;
        emptyVect.resize(nTstep, nanf(""));
        smrydata.push_back(emptyVect);
    }

    //Opm::Hdf5IO::write_2d_hdf5<float>(file_id, "SMRYDATA", smrydata);
    */

    Opm::Hdf5IO::write_1d_hdf5(file_id, "START_DATE", startd );

    Opm::Hdf5IO::write_1d_hdf5(file_id, "KEYS", keywords );

    Opm::Hdf5IO::write_1d_hdf5(file_id, "UNITS", units );


}

/*
    std::cout << "H5_VERS_MAJOR     :  " << H5_VERS_MAJOR << std::endl;
    std::cout << "H5_VERS_MINOR     :  " << H5_VERS_MINOR << std::endl;
    std::cout << "H5_VERS_RELEASE   :  " << H5_VERS_RELEASE << std::endl;
    std::cout << "H5_VERS_SUBRELEASE:  " << H5_VERS_SUBRELEASE << std::endl;
    std::cout << "H5_VERS_INFO      :  " << H5_VERS_INFO << std::endl;
*/


void set_value_rstep(hid_t dset_id, hid_t fpace_id, hid_t mpace_id, int pos, int value)
{
    hsize_t   dims[1] {1};
    hsize_t   offset[1] {pos};

    int data[1]  {value};

    H5Sselect_hyperslab (fpace_id, H5S_SELECT_SET, offset, NULL, dims, NULL);


    H5Dwrite (dset_id, H5T_NATIVE_INT, mpace_id, fpace_id, H5P_DEFAULT, data );
    H5Dflush (dset_id);
}


void set_values_ts(hid_t dset_id, hid_t fpace_id, hid_t mpace, int pos, const std::vector<float>& data)
{
    //hsize_t   dims[2] {data.size(), 1};
    hsize_t   dims[2] {1, 1};
    hsize_t   offset[2] {0, pos};
    hsize_t   block[2] {data.size(), 1};

    //H5Sselect_hyperslab (fpace_id, H5S_SELECT_SET, offset, NULL, dims, NULL);
    H5Sselect_hyperslab (fpace_id, H5S_SELECT_SET, offset, NULL, dims, block);


    H5Dwrite (dset_id, H5T_NATIVE_FLOAT, mpace, fpace_id, H5P_DEFAULT, data.data() );
    H5Dflush (dset_id);
}


void add_value_rstep(hid_t dset_id, hid_t& fpace_id, hid_t mpace_id, int pos, int value)
{
    hsize_t   dims[1] {1};
    hsize_t   offset[1] {pos};
    hsize_t   size[1] {1};

    int data[1]  {value};

    fpace_id = H5Dget_space (dset_id);
    H5Sget_simple_extent_dims(fpace_id, dims, NULL);

    dims[0] = dims[0] + 1;

    H5Dset_extent (dset_id, dims);

    fpace_id = H5Dget_space (dset_id);

    //std::cout << "\n > pos: " << pos << " dims[0]  : " << dims[0];
    //std::cout << " offset[0]: " << offset[0] << "\n";

    H5Sselect_hyperslab (fpace_id, H5S_SELECT_SET, offset, NULL, size, NULL);

    H5Dwrite (dset_id, H5T_NATIVE_INT, mpace_id, fpace_id, H5P_DEFAULT, data );
    H5Dflush (dset_id);

}



void add_values_ts(hid_t dset_id, hid_t& fpace_id, hid_t mpace, int pos, const std::vector<float>& data)
{
 // H5Pset_chunk_cache


    //hsize_t   dims[2] {data.size(), 1};
    hsize_t   dims[2] {1, 1};
    hsize_t   offset[2] {0, pos};
    hsize_t   block[2] {data.size(), 1};


    H5Sget_simple_extent_dims(fpace_id, dims, NULL);

    hsize_t   new_size[2];
    new_size[0] = dims[0];
    new_size[1] = dims[1] + 1;

    H5Dset_extent (dset_id, new_size);

    fpace_id = H5Dget_space (dset_id);

    dims[0] = 1;
    dims[1] = 1;

    H5Sselect_hyperslab (fpace_id, H5S_SELECT_SET, offset, NULL, dims, block);

    H5Dwrite (dset_id, H5T_NATIVE_FLOAT, mpace, fpace_id, H5P_DEFAULT, data.data() );
    H5Dflush (dset_id);
}

void test_hyperlab(){

    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

    hid_t file_id = H5Fcreate("tjohei.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);

    // from chuncked example
    //hid_t  dset_test5x8 = H5Dcreate2 (file_id, "TEST5X8", H5T_NATIVE_FLOAT, dataspace_id,
    //                     H5P_DEFAULT, prop, H5P_DEFAULT);

    hsize_t dims[2] {5,8};

    hid_t  dataspace_id = H5Screate_simple(2, dims, NULL);

    hid_t  dset_test5x8 = H5Dcreate2(file_id, "TEST5X8", H5T_NATIVE_FLOAT, dataspace_id,
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    //hid_t fpace_id_rstep = H5Dget_space (dset_id_rstep);

    std::vector<float> data = {1,2,3,4,5,6,7,8};
    std::vector<float> data2 = {1,3,5,7};
    std::vector<float> data3 = {1,2,3,4,5};


    hsize_t dims_m[2] {1,5};
    //hsize_t dims_m2[2] {1,4};

    hid_t mpace = H5Screate_simple (2, dims_m, NULL);


    hsize_t offset[2] {0, 2};
    hsize_t count[2] {1, 1};
    hsize_t stride[2] {1, 1};
    hsize_t block[2] {5, 1};

    H5Sselect_hyperslab (dataspace_id, H5S_SELECT_SET, offset, stride, count, block);

    H5Dwrite (dset_test5x8, H5T_NATIVE_FLOAT, mpace, dataspace_id, H5P_DEFAULT, data3.data() );


    H5Fclose(file_id);

}


void test_hyperlab_and_chunking(){

    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

    hid_t file_id = H5Fcreate("tjohei2.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);

    // from chuncked example
    //hid_t  dset_test5x8 = H5Dcreate2 (file_id, "TEST5X8", H5T_NATIVE_FLOAT, dataspace_id,
    //                     H5P_DEFAULT, prop, H5P_DEFAULT);

    hsize_t dims[2] {5,8};
    hsize_t maxdims[2] = {5, H5S_UNLIMITED};
    //hsize_t chunk_dims[2] = {5, 1};
    //hsize_t chunk_dims[2] = {1, 8};
    hsize_t chunk_dims[2] = {1, 10};

    //hid_t  dataspace_id = H5Screate_simple(2, dims, NULL);
    hid_t  dataspace_id = H5Screate_simple(2, dims, maxdims);

    hid_t prop = H5Pcreate (H5P_DATASET_CREATE);
    H5Pset_chunk (prop, 2, chunk_dims);


    hid_t  dset_test5x8 = H5Dcreate2(file_id, "TEST5X8", H5T_NATIVE_FLOAT, dataspace_id,
                                  H5P_DEFAULT, prop, H5P_DEFAULT);

    //hid_t fpace_id_rstep = H5Dget_space (dset_id_rstep);

    std::vector<float> data = {1,2,3,4,5,6,7,8};
    std::vector<float> data2 = {1,3,5,7};
    std::vector<float> data3 = {1,2,3,4,5};


    hsize_t dims_m[2] {1,5};
    //hsize_t dims_m2[2] {1,4};

    hid_t mpace = H5Screate_simple (2, dims_m, NULL);


    hsize_t offset[2] {0, 2};
    hsize_t count[2] {1, 1};
    hsize_t stride[2] {1, 1};
    hsize_t block[2] {5, 1};

    H5Sselect_hyperslab (dataspace_id, H5S_SELECT_SET, offset, stride, count, block);

    H5Dwrite (dset_test5x8, H5T_NATIVE_FLOAT, mpace, dataspace_id, H5P_DEFAULT, data3.data() );


    H5Fclose(file_id);

}



int main(int argc, char **argv) {


    //test_hyperlab();
    //test_hyperlab_and_chunking();
    //std::cout << "\n!testing version \n" << std::endl;
    //exit(1);

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
    //h5FileName = h5FileName += ".h5";
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

    int time_ind = -1;

    {
        auto it = std::find(keywords.begin(), keywords.end(), "TIME");
        time_ind = std::distance(keywords.begin(), it);
    }

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

    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);



    hid_t file_id = H5Fcreate(h5FileName.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);


    size_t maxTstep = 3000;

    init_h5_file(file_id, name, keywords, units, startd, maxTstep);


    //hsize_t dims_rstep[1] {maxTstep};
    hsize_t dims_rstep[1] {0};
    hsize_t maxdims_rstep[1] {H5S_UNLIMITED};
    hsize_t chunk_dims_rstep[1] = { 1 };

    hsize_t dims_mspace_rstep[1] {1};
    //hsize_t chunk_dims_rstep[1] = { 120000 };


    hid_t fpace_id_rstep = H5Screate_simple (1, dims_rstep, maxdims_rstep);

    hid_t prop_rstep = H5Pcreate (H5P_DATASET_CREATE);
    H5Pset_chunk (prop_rstep, 1, chunk_dims_rstep);


    hid_t dset_id_rstep = H5Dcreate2 (file_id, "RSTEP", H5T_NATIVE_INT, fpace_id_rstep,
                         H5P_DEFAULT, prop_rstep, H5P_DEFAULT);

    hid_t mpace_rstep = H5Screate_simple (1, dims_rstep, NULL);


    //std::vector<int> default_val_rstep;
    //default_val_rstep.resize(maxTstep, -1);

    //H5Dwrite (dset_id_rstep, H5T_NATIVE_INT, mpace_rstep, fpace_id_rstep, H5P_DEFAULT, default_val_rstep.data() );


    mpace_rstep = H5Screate_simple (1, dims_mspace_rstep, NULL);

    hsize_t dims_smrydata[2] {nVect, 0};

    hsize_t dims_smrydata_mspace[2] {nVect, 1};
    hsize_t maxdims_smrydata[2] = {nVect, H5S_UNLIMITED};

    hsize_t chunk_dims[2] = {5000, 500};


    hid_t fpace_id_smrydata = H5Screate_simple (2, dims_smrydata, maxdims_smrydata);


    hid_t prop = H5Pcreate (H5P_DATASET_CREATE);

    H5Pset_chunk (prop, 2, chunk_dims);

    hid_t dapl_id = H5Pcreate (H5P_DATASET_ACCESS);


    //if (H5Pset_chunk_cache( dapl_id, 30000000, 1000*1024*1024, H5D_CHUNK_CACHE_W0_DEFAULT ) < 0){

    size_t param1 = 30000000;
    size_t param2 = 10000;


    if (H5Pset_chunk_cache( dapl_id, param1*100, param2*1024*1024, H5D_CHUNK_CACHE_W0_DEFAULT ) < 0){
        std::cout << "H5Pset_chunk_cache failed " << std::endl;
        exit(1);
    }

    hid_t dset_id_smrydata = H5Dcreate2 (file_id, "SMRYDATA", H5T_NATIVE_FLOAT, fpace_id_smrydata,
                         H5P_DEFAULT, prop, dapl_id);

    hid_t mpace_smrydata = H5Screate_simple (2, dims_smrydata_mspace, NULL);


    herr_t testswmr = H5Fstart_swmr_write(file_id);

    if (testswmr < 0){
        std::cout << "swmr did not work !" << std::endl;
        exit(1);
    }

    auto lap2 = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = lap2-lap1;

    std::cout << "runtime init h5 file               : " << elapsed_seconds2.count() << " seconds" << std::endl;

    // adding time values

    EclFile unsmry_file(unsmryFileName);

    unsmry_file.loadData("PARAMS");

    auto lap3 = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds3 = lap3-lap2;
    std::cout << "runtime opening and loading unsmry : " << elapsed_seconds1.count() << " seconds\n" << std::endl;

    //auto arrayList = unsmry_file.getList();

    size_t tind = 0;

    double elapsed_writing = 0.0;



    for (size_t n = 0; n < tsData.size(); n++) {
    //for (size_t n = 0; n < 500; n++) {

        std::vector<float> ts_vector = tsData[n];


        std::cout << "adding step: " << std::setw(4) << tind+1;
        std::cout << "/ " << std::setw(4) << nTstep;
        std::cout << "  time: " << std::setw(8) << std::setprecision(2) << std::fixed << ts_vector[time_ind] << " .. " << std::flush;

        auto lap00 = std::chrono::system_clock::now();


        //set_value_rstep(dset_id_rstep, fpace_id_rstep, mpace_rstep ,n, 1);
        add_value_rstep(dset_id_rstep, fpace_id_rstep, mpace_rstep ,n, 1);

        //set_values_ts(dset_id_smrydata, fpace_id_smrydata, mpace_smrydata, n, ts_vector);
        add_values_ts(dset_id_smrydata, fpace_id_smrydata, mpace_smrydata, n, ts_vector);

        /*
        if (n >= maxTstep) {
            //std::cout << "\nneed to increase size, from " << maxTstep << std::flush;
            //std::cout << " to: " << maxTstep*2 << "\n" << std::endl << std::flush;;

            Opm::Hdf5IO::expand_1d_dset<int>(file_id, "RSTEP", 2, -1);
            Opm::Hdf5IO::expand_2d_dset<float>(file_id, "SMRYDATA", 2, nanf(""));

            maxTstep = maxTstep * 2;
        }

        Opm::Hdf5IO::set_value_for_1d_hdf5<int>(file_id, "RSTEP", n, 1);
        Opm::Hdf5IO::set_value_for_2d_hdf5<float>(file_id, "SMRYDATA", n, ts_vector);

        // 1 for is report step, 0 = is not report step (only time step)
        //Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "RSTEP", rstep[n]);
        //Opm::Hdf5IO::add_1d_to_2d_hdf5(file_id, "SMRYDATA", ts_vector);
        */

        auto lap01 = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_ts = lap01-lap00;

        elapsed_writing = elapsed_writing + static_cast<double>(elapsed_ts.count());

        std::cout << " + " << std::setw(8) << std::setprecision(5) << std::fixed << elapsed_ts.count();
        std::cout << " s,  total writing : " << elapsed_writing << " s" << std::endl << std::flush;

        tind++;

        //std::this_thread::sleep_for(std::chrono::microseconds(50000));
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }


    H5Sclose(mpace_rstep);
    H5Sclose(fpace_id_rstep);
    H5Dclose(dset_id_rstep);

    H5Sclose(mpace_smrydata);
    H5Sclose(fpace_id_smrydata);
    H5Dclose(dset_id_smrydata);

    H5Fclose(file_id);



    auto lap4 = std::chrono::system_clock::now();
    std::chrono::duration<double> final_elapsed_tot = lap4-lap3;

    std::cout << "\nFinished, total elapsed: " << final_elapsed_tot.count() << "\n\n";


    return 0;
}
