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

#ifndef OPM_IO_HDF5UTIL_HPP
#define OPM_IO_HDF5UTIL_HPP


#include <vector>
#include "hdf5.h"    // C lib
#include <string>


namespace Opm {   namespace Hdf5IO {

    void  write_str_variable(hid_t file_id, const char* data_set_name, const std::string& variable);

    void write_array_1d(hid_t file_id, const char* data_set_name, hid_t datatype_id, const void * data, size_t size);

    template <typename T>
    void write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<T>& dataVect );

    template <typename T>
    void write_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<std::vector<T>>& dataVect );

    template <typename T>
    std::vector<T> get_1d_hdf5(hid_t file_id, const std::string& data_set_name);

    H5T_class_t open_1d_dset(hid_t file_id, const char* name, hid_t& dataset_id, hid_t& memspace,
                             hid_t& dataspace, size_t& size, size_t& size_e);


    template <typename T>
    std::vector<T> get_1d_from_2d_hdf5(hid_t file_id, const std::string& data_set_name, int vInd, int nTSteps);


}  } // namespace Opm::Hdf5IO

#endif // OPM_IO_HDF5UTIL_HPP
