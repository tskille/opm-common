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
#include <array>
#include "hdf5.h"    // C lib
#include <string>

#include <iostream>


namespace Opm {   namespace Hdf5IO {

    void  write_str_variable(hid_t file_id, const std::string& data_set_name, const std::string& variable);
    std::string  read_str_variable(hid_t file_id, const std::string& data_set_name);

    template <typename T>
    void write_1d_hdf5(hid_t file_id, const std::string& data_set_name,
                   const std::vector<T>& dataVect, bool unlimited = false );

    template <typename T>
    void set_value_for_1d_hdf5(hid_t file_id, const std::string& data_set_name, size_t pos, T value);

    template <typename T>
    void add_value_to_1d_hdf5(hid_t file_id, const std::string& data_set_name, T value);

    template <typename T>
    void set_value_for_2d_hdf5(hid_t file_id, const std::string& data_set_name, size_t pos, const std::vector<T>& data);


    template <typename T>
    void write_2d_hdf5(hid_t file_id, const std::string& data_set_name, const std::vector<std::vector<T>>& dataVect,
                       bool unlimited2 = false, std::array<int, 2> chunk_size={0,0});

    template <typename T>
    void add_1d_to_2d_hdf5(hid_t file_id, const std::string& data_set_name, const std::vector<T>& vectData);

    template <typename T>
    std::vector<T> get_1d_hdf5(hid_t file_id, const std::string& data_set_name);

    template <typename T>
    std::vector<std::vector<T>> get_2d_hdf5(hid_t file_id, const std::string& data_set_name, int size = -1);

    template <typename T>
    std::vector<T> get_1d_from_2d_hdf5(hid_t file_id, const std::string& data_set_name, int vInd, int size = -1 );


    template <typename T>
    void expand_1d_dset(hid_t file_id, const std::string& data_set_name, size_t increase_factor, T notUsedValue)
    {
        std::cout << "updating " << data_set_name << " .. ";

        std::vector<T> data = get_1d_hdf5<T>(file_id, data_set_name);
        std::vector<T> new_chunk;

        size_t new_size = data.size() * increase_factor;
        new_chunk.resize(new_size - data.size(), notUsedValue);

        data.insert(data.end(), new_chunk.begin(), new_chunk.end());

        H5Ldelete(file_id, data_set_name.c_str(), H5P_DEFAULT );

        Opm::Hdf5IO::write_1d_hdf5<T>(file_id,data_set_name.c_str(),data);

        std::cout << " ok ? " << std::endl;
    }


    template <typename T>
    void expand_2d_dset(hid_t file_id, const std::string& data_set_name, size_t increase_factor, T notUsedValue)
    {
        std::cout << "updating " << data_set_name << " .. ";

        auto data2d = get_2d_hdf5<T>(file_id, data_set_name);

        size_t dim_d2 = data2d[0].size();
        size_t n_extra = dim_d2*(increase_factor-1);

        std::vector<T> new_chunk;
        new_chunk.resize(n_extra, notUsedValue);

        for (size_t n=0; n < data2d.size(); n++)
            data2d[n].insert(data2d[n].end(), new_chunk.begin(), new_chunk.end());

        H5Ldelete(file_id, data_set_name.c_str(), H5P_DEFAULT );

        Opm::Hdf5IO::write_2d_hdf5<T>(file_id,data_set_name.c_str(), data2d);
        std::cout << " ok ? " << std::endl;

    }

}  } // namespace Opm::Hdf5IO

#endif // OPM_IO_HDF5UTIL_HPP
