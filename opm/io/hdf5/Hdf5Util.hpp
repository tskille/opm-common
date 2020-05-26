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

    void  write_str_variable(hid_t file_id, const std::string& data_set_name, const std::string& variable);
    std::string  read_str_variable(hid_t file_id, const std::string& data_set_name);

    template <typename T>
    void write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<T>& dataVect, bool unlimited = false );

    template <typename T>
    void add_value_to_1d_hdf5(hid_t file_id, const std::string& data_set_name, T value);

    template <typename T>
    void write_2d_hdf5(hid_t file_id, const std::string& data_set_name,
                       const std::vector<std::vector<T>>& dataVect, bool unlimited2 = false);

    template <typename T>
    void add_1d_to_2d_hdf5(hid_t file_id, const std::string& data_set_name, const std::vector<T>& vectData);

    template <typename T>
    std::vector<T> get_1d_hdf5(hid_t file_id, const std::string& data_set_name);

    template <typename T>
    std::vector<std::vector<T>> get_2d_hdf5(hid_t file_id, const std::string& data_set_name);

    template <typename T>
    std::vector<T> get_1d_from_2d_hdf5(hid_t file_id, const std::string& data_set_name, int vInd);

}  } // namespace Opm::Hdf5IO

#endif // OPM_IO_HDF5UTIL_HPP
