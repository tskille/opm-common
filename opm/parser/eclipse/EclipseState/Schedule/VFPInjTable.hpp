/*
  Copyright 2015 SINTEF ICT, Applied Mathematics.

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


#ifndef OPM_PARSER_ECLIPSE_ECLIPSESTATE_TABLES_VFPINJTABLE_HPP_
#define OPM_PARSER_ECLIPSE_ECLIPSESTATE_TABLES_VFPINJTABLE_HPP_

#include <opm/parser/eclipse/Deck/Deck.hpp>


#include <boost/multi_array.hpp>


namespace Opm {

    class DeckKeyword;

class VFPInjTable {
public:
    typedef boost::multi_array<double, 2> array_type;
    typedef boost::array<array_type::index, 2> extents;

    enum FLO_TYPE {
        FLO_OIL=1,
        FLO_WAT,
        FLO_GAS,
        FLO_INVALID
    };

    inline VFPInjTable() : m_table_num(-1),
            m_datum_depth(-1),
            m_flo_type(FLO_INVALID) {

    }
  VFPInjTable(int table_num,
              double datum_depth,
              FLO_TYPE flo_type,
              const std::vector<double>& flo_data,
              const std::vector<double>& thp_data,
              const array_type& data);
        void init(int table_num,
                double datum_depth,
                FLO_TYPE flo_type,
                const std::vector<double>& flo_data,
                const std::vector<double>& thp_data,
                const array_type& data);

    VFPInjTable(const DeckKeyword& table, const UnitSystem& deck_unit_system);

    inline int getTableNum() const {
        return m_table_num;
    }

    inline double getDatumDepth() const {
        return m_datum_depth;
    }

    inline FLO_TYPE getFloType() const {
        return m_flo_type;
    }

    inline const std::vector<double>& getFloAxis() const {
        return m_flo_data;
    }

    inline const std::vector<double>& getTHPAxis() const {
        return m_thp_data;
    }

    /**
     * Returns the data of the table itself. The data is ordered so that
     *
     * table = getTable();
     * bhp = table[thp_idx][flo_idx];
     *
     * gives the bottom hole pressure value in the table for the coordinate
     * given by
     * flo_axis = getFloAxis();
     * thp_axis = getTHPAxis();
     *
     * flo_coord = flo_axis(flo_idx);
     * thp_coord = thp_axis(thp_idx);
     */
    inline const array_type& getTable() const {
        return m_data;
    }

private:

    int m_table_num;
    double m_datum_depth;
    FLO_TYPE m_flo_type;

    std::vector<double> m_flo_data;
    std::vector<double> m_thp_data;


    array_type m_data;
    void check();

    static FLO_TYPE getFloType(std::string flo_string);

    static void scaleValues(std::vector<double>& values,
                            const double& scaling_factor);

    static void convertFloToSI(const FLO_TYPE& type,
                            std::vector<double>& values,
                            const UnitSystem& unit_system);
    static void convertTHPToSI(std::vector<double>& values,
                            const UnitSystem& unit_system);
};


}


#endif
