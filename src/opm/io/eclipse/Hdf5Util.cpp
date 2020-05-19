/*
   Copyright 2019 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <opm/io/eclipse/Hdf5Util.hpp>

#include <type_traits>
#include <stdexcept>

#include <iostream>

void  Opm::Hdf5IO::write_array_1d(hid_t file_id, const char* data_set_name, hid_t datatype_id, const void * data, size_t size)
{
    hsize_t dims[1] {size};

    hid_t dataspace_id = H5Screate_simple(1, dims, NULL);

    hid_t dataset_id = H5Dcreate2(file_id, data_set_name, datatype_id, dataspace_id,
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    H5Dwrite(dataset_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
}

template <>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<int>& dataVect )
{
    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), H5T_NATIVE_INT, dataVect.data(), dataVect.size());
}

template <>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<float>& dataVect )
{
    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), H5T_NATIVE_FLOAT, dataVect.data(), dataVect.size());
}

template <>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<double>& dataVect )
{
    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), H5T_NATIVE_DOUBLE, dataVect.data(), dataVect.size());
}


template<>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<std::string>& dataVect )
{
    std::vector<const char*> arr_c_str;
    hid_t dataspace_id, datatype_id, dataset_id;

    for (unsigned ii = 0; ii < dataVect.size(); ++ii)
        arr_c_str.push_back(dataVect[ii].c_str());

    datatype_id = H5Tcopy (H5T_C_S1);
    H5Tset_size (datatype_id, H5T_VARIABLE);

    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), datatype_id, arr_c_str.data(), arr_c_str.size());
}

template<>
void Opm::Hdf5IO::write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<bool>& dataVect )
{
    std::vector<int> intVect(dataVect.size(), 1);

    for (size_t n=0; n < dataVect.size(); n++)
        dataVect[n] ? intVect[n]=1 :  intVect[n]=0;

    Opm::Hdf5IO::write_array_1d(file_id, data_set_name.c_str(), H5T_NATIVE_INT, intVect.data(), intVect.size());
}


template <>
void Opm::Hdf5IO::write_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<std::vector<float>>& dataVect )
{
    size_t nx = dataVect.size();
    size_t ny = dataVect[0].size();

    hsize_t     dims[2] {nx, ny};              // dataset dimensions

    hid_t dataspace_id = H5Screate_simple(2, dims, NULL);

    hid_t dataset_id = H5Dcreate2(file_id, data_set_name.c_str(), H5T_NATIVE_FLOAT, dataspace_id,
                                  H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    float * data = new float[nx*ny];

    for (size_t n = 0; n < nx; n ++)
        for (size_t m = 0; m < ny; m ++)
            data[ m + n *ny] = dataVect[n][m];

    H5Dwrite(dataset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    delete[] data;

    H5Sclose(dataspace_id);
    H5Dclose(dataset_id);
}

// -- herfra

H5T_class_t Opm::Hdf5IO::open_1d_dset(hid_t file_id, const char* name, hid_t& dataset_id, hid_t& memspace, hid_t& dataspace,
                         size_t& size, size_t& size_e)
{
    // Turns automatic error printing on or off.
    herr_t status = H5Eset_auto2(NULL, NULL, NULL);

    dataset_id = H5Dopen2(file_id, name, H5P_DEFAULT);

    if( dataset_id < 0)
        throw std::invalid_argument("dataset not found in file");

    hid_t datatype  = H5Dget_type(dataset_id);

    H5T_class_t t_class   = H5Tget_class(datatype);

    size_e = H5Tget_size (datatype);

    H5Tclose(datatype);

    dataspace = H5Dget_space(dataset_id);

    int rank  = H5Sget_simple_extent_ndims(dataspace);

    if (rank != 1)
        throw std::invalid_argument("dataset found, but this is not of 1D");

    hsize_t dims[rank];

    int status_n  = H5Sget_simple_extent_dims(dataspace, dims, NULL);
    size = dims[0];

    memspace = H5Screate_simple(rank, dims, NULL);

    return t_class;
}


template<>
std::vector<int> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);

    if ((t_class != H5T_INTEGER) || (size_e != 4))
        throw std::invalid_argument("dataset found, but this has wrong data type");

    int data_out[size]; //

    herr_t status = H5Dread(dataset_id, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, data_out);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    std::vector<int> data_vect(data_out, data_out + size);

    return data_vect;
}

template<>
std::vector<float> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);


    if ((t_class != H5T_FLOAT) || (size_e != 4))
        throw std::invalid_argument("dataset found, but this has wrong data type");

    float data_out[size]; //

    herr_t status = H5Dread(dataset_id, H5T_NATIVE_FLOAT, memspace, dataspace, H5P_DEFAULT, data_out);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    std::vector<float> data_vect(data_out, data_out + size);

    return data_vect;
}

template<>
std::vector<double> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);

    if ((t_class != H5T_FLOAT) || (size_e != 8))
        throw std::invalid_argument("dataset found, but this has wrong data type");

    double data_out[size]; //

    herr_t status = H5Dread(dataset_id, H5T_NATIVE_DOUBLE, memspace, dataspace, H5P_DEFAULT, data_out);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    std::vector<double> data_vect(data_out, data_out + size);

    return data_vect;
}

template<>
std::vector<std::string> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);

    if (t_class != H5T_STRING)
        throw std::invalid_argument("dataset found, but this has wrong data type");

    std::vector<const char*> tmpvect( size, NULL );

    hid_t datatype  = H5Dget_type(dataset_id);
    herr_t status = H5Dread(dataset_id, datatype, memspace, dataspace, H5P_DEFAULT, tmpvect.data());

    std::vector<std::string> data_vect;
    data_vect.reserve(size);

    for(size_t n = 0; n < size; n++)
        data_vect.push_back(tmpvect[n]);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    return data_vect;
}

template<>
std::vector<bool> Opm::Hdf5IO::get_1d_hdf5(hid_t file_id, const std::string& data_set_name)
{
    hid_t dataset_id, memspace, dataspace;
    size_t size, size_e;

    H5T_class_t t_class = open_1d_dset(file_id, data_set_name.c_str(), dataset_id, memspace, dataspace, size, size_e);

    if (t_class != H5T_INTEGER)
        throw std::invalid_argument("dataset found, but this has wrong data type");

    int out_data[size];

    herr_t status = H5Dread(dataset_id, H5T_NATIVE_INT, memspace, dataspace, H5P_DEFAULT, out_data);

    std::vector<bool> data_vect;
    data_vect.reserve(size);

    for(size_t n = 0; n < size; n++)
        if (out_data[n]==0)
            data_vect.push_back(false);
        else
            data_vect.push_back(true);

    H5Sclose(dataspace);
    H5Sclose(memspace);
    H5Dclose(dataset_id);

    //delete[] out_data;

    return data_vect;
}

template<>
std::vector<float> Opm::Hdf5IO::get_1d_from_2d_hdf5(hid_t file_id, const std::string& data_set_name, int vInd, int nTSteps)
{
    hid_t dataset_id, dataspace;
    size_t size, size_e;

    dataset_id = H5Dopen2(file_id, data_set_name.c_str(), H5P_DEFAULT);

    dataspace = H5Dget_space(dataset_id);

    int rank  = H5Sget_simple_extent_ndims(dataspace);

    hsize_t dims[rank];

    if (rank != 2)
        throw std::invalid_argument("dimension of SMRYDATA should be 2");

    int status_n  = H5Sget_simple_extent_dims(dataspace, dims, NULL);

    //  Define hyperslab in the dataset;

    hsize_t  offset[2];   // hyperslab offset in the file
    hsize_t  count[2];    // size of the hyperslab in the file

    offset[0] = vInd;
    offset[1] = 0;

    count[0]  = 1;
    count[1]  = nTSteps;

    hsize_t  dims2[2]  = {1, nTSteps};

    hid_t memspace = H5Screate_simple(2, dims2, NULL);

    H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);

    float data_out[nTSteps]; //

    herr_t  status = H5Dread(dataset_id, H5T_NATIVE_FLOAT, memspace, dataspace, H5P_DEFAULT, data_out);

    std::vector<float> data_vect(data_out, data_out + nTSteps);

    H5Sclose(memspace);
    H5Sclose(dataspace);
    H5Dclose(dataset_id);


    return data_vect;
}


