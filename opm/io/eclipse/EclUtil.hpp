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

#ifndef OPM_IO_ECLUTIL_HPP
#define OPM_IO_ECLUTIL_HPP

#include <opm/io/eclipse/EclIOdata.hpp>

#include <string>
#include <tuple>

namespace Opm { namespace EclIO {

    int flipEndianInt(int num);
    long int flipEndianLongInt(long int num);
    float flipEndianFloat(float num);
    double flipEndianDouble(double num);

    std::tuple<int, int> block_size_data_binary(eclArrType arrType);
    std::tuple<int, int, int> block_size_data_formatted(eclArrType arrType);

    std::string trimr(const std::string &str1);

}} // namespace Opm::EclIO

#endif // OPM_IO_ECLUTIL_HPP
