/*
   +   Copyright 2019 Equinor ASA.
   +
   +   This file is part of the Open Porous Media project (OPM).
   +
   +   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   +   the Free Software Foundation, either version 3 of the License, or
   +   (at your option) any later version.
   +
   +   OPM is distributed in the hope that it will be useful,
   +   but WITHOUT ANY WARRANTY; without even the implied warranty of
   +   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   +   GNU General Public License for more details.
   +
   +   You should have received a copy of the GNU General Public License
   +   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   +   */

#include "config.h"
#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <tuple>
#include <cmath>
#include <numeric>
#include "hdf5.h"    // C lib

#include <opm/io/hdf5/Hdf5Util.hpp>
#include "WorkArea.cpp"

#include <opm/io/eclipse/EclOutput.hpp>

#include <opm/common/utility/FileSystem.hpp>

#define BOOST_TEST_MODULE Test EclIO
#include <boost/test/unit_test.hpp>



//using namespace Opm::EclIO;

template <typename T>
void print_vector(std::vector<T>& vect)
{
    std::cout << " : ";
    for (auto v : vect)
        std::cout << " " << std::setw(5) << v;

    std::cout << std::endl;
}




template<typename InputIterator1, typename InputIterator2>
bool
range_equal(InputIterator1 first1, InputIterator1 last1,
            InputIterator2 first2, InputIterator2 last2)
{
    while(first1 != last1 && first2 != last2)
    {
        if(*first1 != *first2) return false;
        ++first1;
        ++first2;
    }
    return (first1 == last1) && (first2 == last2);
}

bool compare_files(const std::string& filename1, const std::string& filename2)
{
    std::ifstream file1(filename1);
    std::ifstream file2(filename2);

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    std::istreambuf_iterator<char> end;

    return range_equal(begin1, end, begin2, end);
}

template <typename T>
bool operator==(const std::vector<T> & t1, const std::vector<T> & t2)
{
    return std::equal(t1.begin(), t1.end(), t2.begin(), t2.end());
}


BOOST_AUTO_TEST_CASE(test_string_variable) {

    WorkArea wa;
    std::string filename = wa.currentWorkingDirectory() + "/test.h5";

    std::string str_var = "DALGLISH";

    hid_t file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    Opm::Hdf5IO::write_str_variable(file_id, "TEST_STR_VAR", str_var);
    H5Fclose(file_id);

    file_id = H5Fopen( filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    std::string test_str = Opm::Hdf5IO::read_str_variable(file_id, "TEST_STR_VAR");

    BOOST_CHECK_EQUAL(str_var, test_str);
}


BOOST_AUTO_TEST_CASE(test_1d_arrays_fixed) {

    WorkArea wa;
    std::string filename = wa.currentWorkingDirectory() + "/test.h5";

    std::vector<float> ts0 = {0.0, 0.1, 0.2, 0.3, 0.4};
    std::vector<double> dts0 = {0.000, 0.111, 0.222, 0.333, 0.444};
    std::vector<int> its0 = {0,1,2,3,4};
    std::vector<std::string> sts0 = {"v0T0","v1T0","v2T0","v3T0","v4T0"};

    hid_t file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    Opm::Hdf5IO::write_1d_hdf5(file_id, "float_data", ts0);
    Opm::Hdf5IO::write_1d_hdf5(file_id, "double_data", dts0);
    Opm::Hdf5IO::write_1d_hdf5(file_id, "int_data", its0);
    Opm::Hdf5IO::write_1d_hdf5(file_id, "string_data", sts0);

    H5Fclose(file_id);

    file_id = H5Fopen( filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    auto test_f = Opm::Hdf5IO::get_1d_hdf5<float>(file_id, "float_data");
    auto test_d = Opm::Hdf5IO::get_1d_hdf5<double>(file_id, "double_data");
    auto test_i = Opm::Hdf5IO::get_1d_hdf5<int>(file_id, "int_data");
    auto test_s = Opm::Hdf5IO::get_1d_hdf5<std::string>(file_id, "string_data");

    BOOST_CHECK_EQUAL(ts0 == test_f, true);
    BOOST_CHECK_EQUAL(dts0 == test_d, true);
    BOOST_CHECK_EQUAL(its0 == test_i, true);
    BOOST_CHECK_EQUAL(sts0 == test_s, true);
}

BOOST_AUTO_TEST_CASE(test_1d_arrays_unlim) {

    WorkArea wa;
    std::string filename = wa.currentWorkingDirectory() + "/test.h5";

    std::vector<float> ts0 = {0.0, 0.1, 0.2, 0.3, 0.4};
    std::vector<double> dts0 = {0.000, 0.111, 0.222, 0.333, 0.444};
    std::vector<int> its0 = {0,1,2,3,4};
    std::vector<std::string> sts0 = {"v0T0","v1T0","v2T0","v3T0","v4T0"};

    hid_t file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    Opm::Hdf5IO::write_1d_hdf5<float>(file_id, "float_data", {}, true);
    Opm::Hdf5IO::write_1d_hdf5<double>(file_id, "double_data", {}, true);
    Opm::Hdf5IO::write_1d_hdf5<int>(file_id, "int_data", {}, true);
    Opm::Hdf5IO::write_1d_hdf5<std::string>(file_id, "string_data", {}, true);

    for (auto fval : ts0)
        Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "float_data", fval);

    for (auto dval : dts0)
        Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "double_data", dval);

    for (auto ival : its0)
        Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "int_data", ival);

    for (auto sval : sts0)
        Opm::Hdf5IO::add_value_to_1d_hdf5(file_id, "string_data", sval);

    H5Fclose(file_id);

    file_id = H5Fopen( filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    auto test_f = Opm::Hdf5IO::get_1d_hdf5<float>(file_id, "float_data");
    auto test_d = Opm::Hdf5IO::get_1d_hdf5<double>(file_id, "double_data");
    auto test_i = Opm::Hdf5IO::get_1d_hdf5<int>(file_id, "int_data");
    auto test_s = Opm::Hdf5IO::get_1d_hdf5<std::string>(file_id, "string_data");

    BOOST_CHECK_EQUAL(ts0 == test_f, true);
    BOOST_CHECK_EQUAL(dts0 == test_d, true);
    BOOST_CHECK_EQUAL(its0 == test_i, true);
    BOOST_CHECK_EQUAL(sts0 == test_s, true);

    H5Fclose(file_id);
}


BOOST_AUTO_TEST_CASE(test_1d_arrays_swmr) {

    // swmr = single write multiple read. Testing concurrent write and read on same file
    //    writing to  > wfile_id
    //    reading with > rfile_id

    WorkArea wa;
    std::string filename = wa.currentWorkingDirectory() + "/test.h5";

    std::vector<float> ts0 = {0.0, 0.1, 0.2, 0.3, 0.4};
    std::vector<double> dts0 = {0.000, 0.111, 0.222, 0.333, 0.444};
    std::vector<int> its0 = {0,1,2,3,4};
    std::vector<std::string> sts0 = {"v0T0","v1T0","v2T0","v3T0","v4T0"};

    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

    hid_t wfile_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);

    Opm::Hdf5IO::write_1d_hdf5<float>(wfile_id, "float_data", {}, true);
    Opm::Hdf5IO::write_1d_hdf5<double>(wfile_id, "double_data", {}, true);
    Opm::Hdf5IO::write_1d_hdf5<int>(wfile_id, "int_data", {}, true);
    Opm::Hdf5IO::write_1d_hdf5<std::string>(wfile_id, "string_data", {}, true);

    // no datasets can be added after starting SWMR, however data can be added to
    // existing data sets

    if (H5Fstart_swmr_write(wfile_id) < 0)
        throw std::runtime_error("failed when starting swmr");

    hid_t rfile_id = H5Fopen( filename.c_str(), H5F_ACC_RDONLY|H5F_ACC_SWMR_READ, H5P_DEFAULT);

    for (size_t n = 0; n < ts0.size(); n++)
    {
        Opm::Hdf5IO::add_value_to_1d_hdf5(wfile_id, "float_data", ts0[n]);
        auto test_f = Opm::Hdf5IO::get_1d_hdf5<float>(rfile_id, "float_data");

        std::vector<float> tmp_float(ts0.begin(), ts0.begin() + n + 1);

        BOOST_CHECK_EQUAL(test_f == tmp_float, true);
    }

    for (size_t n = 0; n < dts0.size(); n++)
    {
        Opm::Hdf5IO::add_value_to_1d_hdf5(wfile_id, "double_data", dts0[n]);
        auto test_d = Opm::Hdf5IO::get_1d_hdf5<double>(rfile_id, "double_data");

        std::vector<double> tmp_double(dts0.begin(), dts0.begin() + n + 1);

        BOOST_CHECK_EQUAL(test_d == tmp_double, true);
    }

    for (size_t n = 0; n < its0.size(); n++)
    {
        Opm::Hdf5IO::add_value_to_1d_hdf5(wfile_id, "int_data", its0[n]);
        auto test_i = Opm::Hdf5IO::get_1d_hdf5<int>(rfile_id, "int_data");

        std::vector<int> tmp_int(its0.begin(), its0.begin() + n + 1);

        BOOST_CHECK_EQUAL(test_i == tmp_int, true);
    }

    for (size_t n = 0; n < sts0.size(); n++)
    {
        Opm::Hdf5IO::add_value_to_1d_hdf5(wfile_id, "string_data", sts0[n]);
        auto test_s = Opm::Hdf5IO::get_1d_hdf5<std::string>(rfile_id, "string_data");

        std::vector<std::string> tmp_string(sts0.begin(), sts0.begin() + n + 1);

        BOOST_CHECK_EQUAL(test_s == tmp_string, true);
    }

    H5Fclose(wfile_id);
    H5Fclose(rfile_id);
}

BOOST_AUTO_TEST_CASE(test_2d_arrays_fixed) {

    WorkArea wa;
    std::string filename = wa.currentWorkingDirectory() + "/test.h5";


    std::vector<std::vector<float>> f_array = {{0.0, 0.1, 0.2, 0.3, 0.4},
                                               {1.0, 1.1, 1.2, 1.3, 1.4},
                                               {2.0, 2.1, 2.2, 2.3, 2.4},
                                               {3.0, 3.1, 3.2, 3.3, 3.4},
                                              };

    std::vector<std::vector<double>> d_array = {{0.000, 0.111, 0.222, 0.333, 0.444},
                                                {1.000, 1.111, 1.222, 1.333, 1.444},
                                                {2.000, 2.111, 2.222, 2.333, 2.444},
                                                {3.000, 3.111, 3.222, 3.333, 3.444},
                                               };

    std::vector<std::vector<int>> i_array = {{0,1,2,3,4,6,7,8,9},
                                             {10,11,12,13,14,16,17,18,19},
                                             {20,21,22,23,24,26,27,28,29},
                                             {30,31,32,33,34,36,37,38,39},
                                            };

    std::vector<std::vector<std::string>> s_array = {{"v0T0","v1T0","v2T0","v3T0","v4T0"},
                                                     {"v0T1","v1T1","v2T1","v3T1","v4T1"},
                                                     {"v0T2","v1T2","v2T2","v3T2","v4T2"},
                                                     {"v0T3","v1T3","v2T3","v3T3","v4T3"},
                                                    };

    hid_t file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    Opm::Hdf5IO::write_2d_hdf5(file_id, "float_2d_data", f_array);
    Opm::Hdf5IO::write_2d_hdf5(file_id, "double_2d_data", d_array);
    Opm::Hdf5IO::write_2d_hdf5(file_id, "int_2d_data", i_array);
    Opm::Hdf5IO::write_2d_hdf5(file_id, "str_2d_data", s_array);

    H5Fclose(file_id);

    file_id = H5Fopen( filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    auto test_f = Opm::Hdf5IO::get_2d_hdf5<float>(file_id, "float_2d_data");
    auto test_d = Opm::Hdf5IO::get_2d_hdf5<double>(file_id, "double_2d_data");
    auto test_i = Opm::Hdf5IO::get_2d_hdf5<int>(file_id, "int_2d_data");
    auto test_s = Opm::Hdf5IO::get_2d_hdf5<std::string>(file_id, "str_2d_data");

    H5Fclose(file_id);

    BOOST_CHECK_EQUAL(test_f.size(), f_array.size());
    BOOST_CHECK_EQUAL(test_d.size(), d_array.size());
    BOOST_CHECK_EQUAL(test_i.size(), i_array.size());
    BOOST_CHECK_EQUAL(test_s.size(), s_array.size());

    for (size_t n = 0; n < f_array.size(); n ++)
        BOOST_CHECK_EQUAL(f_array[n] == test_f[n], true);

    for (size_t n = 0; n < d_array.size(); n ++)
        BOOST_CHECK_EQUAL(d_array[n] == test_d[n], true);

    for (size_t n = 0; n < i_array.size(); n ++)
        BOOST_CHECK_EQUAL(i_array[n] == test_i[n], true);

    for (size_t n = 0; n < s_array.size(); n ++)
        BOOST_CHECK_EQUAL(s_array[n] == test_s[n], true);

}


BOOST_AUTO_TEST_CASE(test_2d_arrays_unlim_2nd_dim) {

    WorkArea wa;
    std::string filename = wa.currentWorkingDirectory() + "/test.h5";
    //std::string filename = "/tmp/tjohei/test.h5";


    std::vector<std::vector<float>> f_array = {{0.0, 0.1, 0.2, 0.3, 0.4},
                                               {1.0, 1.1, 1.2, 1.3, 1.4},
                                               {2.0, 2.1, 2.2, 2.3, 2.4},
                                               {3.0, 3.1, 3.2, 3.3, 3.4},
                                              };

    std::vector<std::vector<double>> d_array = {{0.000, 0.111, 0.222, 0.333, 0.444},
                                                {1.000, 1.111, 1.222, 1.333, 1.444},
                                                {2.000, 2.111, 2.222, 2.333, 2.444},
                                                {3.000, 3.111, 3.222, 3.333, 3.444},
                                               };

    std::vector<std::vector<int>> i_array = {{0, 1, 2, 3, 4, 5, 6, 7, 8},
                                             {10, 11, 12, 13, 14, 15, 16, 17, 18},
                                             {20, 21, 22, 23, 24, 25, 26, 27, 28},
                                             {30, 31, 32, 33, 34, 35, 36, 37, 38},
                                            };

    std::vector<std::vector<std::string>> s_array = {{"v0T0","v1T0","v2T0","v3T0","v4T0"},
                                                     {"v0T1","v1T1","v2T1","v3T1","v4T1"},
                                                     {"v0T2","v1T2","v2T2","v3T2","v4T2"},
                                                     {"v0T3","v1T3","v2T3","v3T3","v4T3"},
                                                    };

    hid_t file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);


    std::vector<std::vector<float>> float2d;
    std::vector<std::vector<double>> double2d;
    std::vector<std::vector<int>> int2d;
    std::vector<std::vector<std::string>> string2d;

    for (size_t n = 0; n < f_array[0].size(); n++)
        float2d.push_back({});

    for (size_t n = 0; n < d_array[0].size(); n++)
        double2d.push_back({d_array[0][n]});

    for (size_t n = 0; n < i_array[0].size(); n++)
        int2d.push_back({});

    for (size_t n = 0; n < s_array[0].size(); n++)
        string2d.push_back({s_array[0][n], s_array[1][n]});


    Opm::Hdf5IO::write_2d_hdf5<float>(file_id, "float_2d_data", float2d, true);
    Opm::Hdf5IO::write_2d_hdf5<double>(file_id, "double_2d_data", double2d, true);
    Opm::Hdf5IO::write_2d_hdf5<int>(file_id, "int_2d_data", int2d, true);
    Opm::Hdf5IO::write_2d_hdf5<std::string>(file_id, "str_2d_data", string2d, true);


    for (size_t n = 0; n < f_array.size(); n++)
        Opm::Hdf5IO::add_1d_to_2d_hdf5(file_id, "float_2d_data", f_array[n]);

    for (size_t n = 1; n < d_array.size(); n++)
        Opm::Hdf5IO::add_1d_to_2d_hdf5(file_id, "double_2d_data", d_array[n]);

    for (size_t n = 0; n < i_array.size(); n++)
        Opm::Hdf5IO::add_1d_to_2d_hdf5(file_id, "int_2d_data", i_array[n]);


    for (size_t n = 2; n < s_array.size(); n++){
        Opm::Hdf5IO::add_1d_to_2d_hdf5(file_id, "str_2d_data", s_array[n]);
    }

    H5Fclose(file_id);


    file_id = H5Fopen( filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    auto test_f = Opm::Hdf5IO::get_2d_hdf5<float>(file_id, "float_2d_data");
    auto test_d = Opm::Hdf5IO::get_2d_hdf5<double>(file_id, "double_2d_data");
    auto test_i = Opm::Hdf5IO::get_2d_hdf5<int>(file_id, "int_2d_data");
    auto test_s = Opm::Hdf5IO::get_2d_hdf5<std::string>(file_id, "str_2d_data");

    H5Fclose(file_id);

    for (size_t n = 0; n < test_f.size(); n ++){
        BOOST_CHECK_EQUAL(test_f[n].size() , f_array.size());
        for (size_t m = 0; m < f_array.size(); m++){
            BOOST_CHECK_EQUAL(test_f[n][m] ,f_array[m][n]);
        }
    }

    for (size_t n = 0; n < test_d.size(); n ++){
        BOOST_CHECK_EQUAL(test_d[n].size() , d_array.size());
        for (size_t m = 0; m < d_array.size(); m++){
            BOOST_CHECK_EQUAL(test_d[n][m] ,d_array[m][n]);
        }
    }

    for (size_t n = 0; n < test_i.size(); n ++){
        BOOST_CHECK_EQUAL(test_i[n].size() , i_array.size());
        for (size_t m = 0; m < d_array.size(); m++){
            BOOST_CHECK_EQUAL(test_i[n][m] ,i_array[m][n]);
        }
    }

    for (size_t n = 0; n < test_s.size(); n ++){
        BOOST_CHECK_EQUAL(test_s[n].size() , s_array.size());
        for (size_t m = 0; m < s_array.size(); m++){
            BOOST_CHECK_EQUAL(test_s[n][m] ,s_array[m][n]);
        }
    }

    // testing |> get_1d_from_2d_hdf5

    file_id = H5Fopen( filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

    for (size_t n = 0; n < f_array[0].size(); n++) {
        auto test_f2 = Opm::Hdf5IO::get_1d_from_2d_hdf5<float>(file_id, "float_2d_data", n);
        for (size_t m = 0; m < f_array.size(); m++)
            BOOST_CHECK_EQUAL(test_f2[m] ,f_array[m][n]);
    }

    for (size_t n = 0; n < d_array[0].size(); n++) {
        auto test_d2 = Opm::Hdf5IO::get_1d_from_2d_hdf5<double>(file_id, "double_2d_data", n);
        for (size_t m = 0; m < d_array.size(); m++)
            BOOST_CHECK_EQUAL(test_d2[m] ,d_array[m][n]);
    }

    for (size_t n = 0; n < i_array[0].size(); n++) {
        auto test_i2 = Opm::Hdf5IO::get_1d_from_2d_hdf5<int>(file_id, "int_2d_data", n);
        for (size_t m = 0; m < i_array.size(); m++)
            BOOST_CHECK_EQUAL(test_i2[m] ,i_array[m][n]);
    }

    H5Fclose(file_id);


}


BOOST_AUTO_TEST_CASE(test_2d_arrays_swmr) {

    WorkArea wa;
    std::string filename = wa.currentWorkingDirectory() + "/test.h5";
    //std::string filename = "/tmp/tjohei/test.h5";


    std::vector<std::vector<float>> f_array = {{0.0, 0.1, 0.2, 0.3, 0.4},
                                               {1.0, 1.1, 1.2, 1.3, 1.4},
                                               {2.0, 2.1, 2.2, 2.3, 2.4},
                                               {3.0, 3.1, 3.2, 3.3, 3.4},
                                              };

    std::vector<std::vector<double>> d_array = {{0.000, 0.111, 0.222, 0.333, 0.444},
                                                {1.000, 1.111, 1.222, 1.333, 1.444},
                                                {2.000, 2.111, 2.222, 2.333, 2.444},
                                                {3.000, 3.111, 3.222, 3.333, 3.444},
                                               };

    std::vector<std::vector<int>> i_array = {{0, 1, 2, 3, 4, 5, 6, 7, 8},
                                             {10, 11, 12, 13, 14, 15, 16, 17, 18},
                                             {20, 21, 22, 23, 24, 25, 26, 27, 28},
                                             {30, 31, 32, 33, 34, 35, 36, 37, 38},
                                            };

    std::vector<std::vector<std::string>> s_array = {{"v0T0","v1T0","v2T0","v3T0","v4T0"},
                                                     {"v0T1","v1T1","v2T1","v3T1","v4T1"},
                                                     {"v0T2","v1T2","v2T2","v3T2","v4T2"},
                                                     {"v0T3","v1T3","v2T3","v3T3","v4T3"},
                                                    };

    hid_t fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_libver_bounds(fapl_id, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

    hid_t wfile_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);


    std::vector<std::vector<float>> float2d;
    std::vector<std::vector<double>> double2d;
    std::vector<std::vector<int>> int2d;
    std::vector<std::vector<std::string>> string2d;

    for (size_t n = 0; n < f_array[0].size(); n++)
        float2d.push_back({});

    for (size_t n = 0; n < d_array[0].size(); n++)
        double2d.push_back({});

    for (size_t n = 0; n < i_array[0].size(); n++)
        int2d.push_back({});

    for (size_t n = 0; n < s_array[0].size(); n++)
        string2d.push_back({});


    Opm::Hdf5IO::write_2d_hdf5<float>(wfile_id, "float_2d_data", float2d, true);
    Opm::Hdf5IO::write_2d_hdf5<double>(wfile_id, "double_2d_data", double2d, true);
    Opm::Hdf5IO::write_2d_hdf5<int>(wfile_id, "int_2d_data", int2d, true);
    Opm::Hdf5IO::write_2d_hdf5<std::string>(wfile_id, "str_2d_data", string2d, true);


    if (H5Fstart_swmr_write(wfile_id) < 0)
        throw std::runtime_error("failed when starting swmr");

    hid_t rfile_id = H5Fopen( filename.c_str(), H5F_ACC_RDONLY|H5F_ACC_SWMR_READ, H5P_DEFAULT);

    for (size_t n = 0; n < f_array.size(); n++) {
        Opm::Hdf5IO::add_1d_to_2d_hdf5(wfile_id, "float_2d_data", f_array[n]);

        for (size_t vInd=0; vInd < f_array[0].size(); vInd++) {

            auto test_f = Opm::Hdf5IO::get_1d_from_2d_hdf5<float>(rfile_id, "float_2d_data", vInd);

            BOOST_CHECK_EQUAL(n + 1, test_f.size());

            for (size_t m = 0; m < test_f.size(); m++)
                BOOST_CHECK_EQUAL(test_f[m], f_array[m][vInd]);
        }
    }

    H5Fclose(wfile_id);
    H5Fclose(rfile_id);
}

