#define BOOST_TEST_MODULE UDQ_Data

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/output/eclipse/AggregateUDQData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/output/eclipse/InteHEAD.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/DoubHEAD.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>

//#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
//#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include <opm/io/eclipse/OutputStream.hpp>

#include <stdexcept>
#include <utility>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {
    Opm::Deck first_sim()
    {
        // Mostly copy of tests/FIRST_SIM.DATA
        const std::string input = std::string {
            R"~(
RUNSPEC

TITLE
2 PRODUCERS  AND INJECTORS, 2 WELL GROUPS AND ONE INTERMEDIATE GROUP LEVEL  BELOW THE FIELD LEVEL

DIMENS
 10  5  10  /


OIL

WATER

GAS

DISGAS

FIELD

TABDIMS
 1  1  15  15  2  15  /

EQLDIMS
 2  /

WELLDIMS
 4  20  4  2  /

UNIFIN
UNIFOUT

FMTIN
FMTOUT
-- Dimensions for used defined quantity facility
-- max functions permitted in a quantity definition
-- max arguments permitted in a quantity definition
-- max user defined connection quantities
-- max user defined field quantities
-- max user defined group quantities
-- max user defined region quantities
-- max user defined segment quantities
-- max user defined well quantities
-- max user defined aquifer quantities
-- max user defined block quantities
-- whether new randon number generator seed computed for restart runs
UDQDIMS
 50 25 0 50 50 0 0 50 0 20 /

-- Dimensions for the user defined arguments facility
-- number of keyword arguments in which UDQs replace numerical values
-- ratained for back-compatibility
-- total number of unique instances in which a UDQ is used in a keyword argument
UDADIMS
 10   1*  10 /

START
 1 'JAN' 2015 /

-- RPTRUNSP

GRID        =========================================================

--NOGGF
BOX
 1 10 1 5 1 1 /

TOPS
50*7000 /

BOX
1 10  1 5 1 10 /

DXV
10*100 /
DYV
5*100  /
DZV
2*20 100 7*20 /

EQUALS
-- 'DX'     100  /
-- 'DY'     100  /
 'PERMX'  50   /
 'PERMZ'  5   /
-- 'DZ'     20   /
 'PORO'   0.2  /
-- 'TOPS'   7000   1 10  1 5  1 1  /
-- 'DZ'     100    1 10  1 5  3 3  /
-- 'PORO'   0.0    1 10  1 5  3 3  /
 /

COPY
  PERMX PERMY /
 /

RPTGRID
  -- Report Levels for Grid Section Data
  --
 /

PROPS       ==========================================================

-- WATER RELATIVE PERMEABILITY AND CAPILLARY PRESSURE ARE TABULATED AS
-- A FUNCTION OF WATER SATURATION.
--
--  SWAT   KRW   PCOW
SWFN

    0.12  0       0
    1.0   0.00001 0  /

-- SIMILARLY FOR GAS
--
--  SGAS   KRG   PCOG
SGFN

    0     0       0
    0.02  0       0
    0.05  0.005   0
    0.12  0.025   0
    0.2   0.075   0
    0.25  0.125   0
    0.3   0.19    0
    0.4   0.41    0
    0.45  0.6     0
    0.5   0.72    0
    0.6   0.87    0
    0.7   0.94    0
    0.85  0.98    0
    1.0   1.0     0
/

-- OIL RELATIVE PERMEABILITY IS TABULATED AGAINST OIL SATURATION
-- FOR OIL-WATER AND OIL-GAS-CONNATE WATER CASES
--
--  SOIL     KROW     KROG
SOF3

    0        0        0
    0.18     0        0
    0.28     0.0001   0.0001
    0.38     0.001    0.001
    0.43     0.01     0.01
    0.48     0.021    0.021
    0.58     0.09     0.09
    0.63     0.2      0.2
    0.68     0.35     0.35
    0.76     0.7      0.7
    0.83     0.98     0.98
    0.86     0.997    0.997
    0.879    1        1
    0.88     1        1    /


-- PVT PROPERTIES OF WATER
--
--    REF. PRES. REF. FVF  COMPRESSIBILITY  REF VISCOSITY  VISCOSIBILITY
PVTW
       4014.7     1.029        3.13D-6           0.31            0 /

-- ROCK COMPRESSIBILITY
--
--    REF. PRES   COMPRESSIBILITY
ROCK
        14.7          3.0D-6          /

-- SURFACE DENSITIES OF RESERVOIR FLUIDS
--
--        OIL   WATER   GAS
DENSITY
         49.1   64.79  0.06054  /

-- PVT PROPERTIES OF DRY GAS (NO VAPOURISED OIL)
-- WE WOULD USE PVTG TO SPECIFY THE PROPERTIES OF WET GAS
--
--   PGAS   BGAS   VISGAS
PVDG
     14.7 166.666   0.008
    264.7  12.093   0.0096
    514.7   6.274   0.0112
   1014.7   3.197   0.014
   2014.7   1.614   0.0189
   2514.7   1.294   0.0208
   3014.7   1.080   0.0228
   4014.7   0.811   0.0268
   5014.7   0.649   0.0309
   9014.7   0.386   0.047   /

-- PVT PROPERTIES OF LIVE OIL (WITH DISSOLVED GAS)
-- WE WOULD USE PVDO TO SPECIFY THE PROPERTIES OF DEAD OIL
--
-- FOR EACH VALUE OF RS THE SATURATION PRESSURE, FVF AND VISCOSITY
-- ARE SPECIFIED. FOR RS=1.27 AND 1.618, THE FVF AND VISCOSITY OF
-- UNDERSATURATED OIL ARE DEFINED AS A FUNCTION OF PRESSURE. DATA
-- FOR UNDERSATURATED OIL MAY BE SUPPLIED FOR ANY RS, BUT MUST BE
-- SUPPLIED FOR THE HIGHEST RS (1.618).
--
--   RS      POIL  FVFO  VISO
PVTO
    0.001    14.7 1.062  1.04    /
    0.0905  264.7 1.15   0.975   /
    0.18    514.7 1.207  0.91    /
    0.371  1014.7 1.295  0.83    /
    0.636  2014.7 1.435  0.695   /
    0.775  2514.7 1.5    0.641   /
    0.93   3014.7 1.565  0.594   /
    1.270  4014.7 1.695  0.51
           5014.7 1.671  0.549
           9014.7 1.579  0.74    /
    1.618  5014.7 1.827  0.449
           9014.7 1.726  0.605   /
/


RPTPROPS
-- PROPS Reporting Options
--
/

REGIONS    ===========================================================


FIPNUM

  100*1
  400*2
/

EQLNUM

  100*1
  400*2
/

RPTREGS

    /

SOLUTION    ============================================================

EQUIL
 7020.00 2700.00 7990.00  .00000 7020.00  .00000     0      0       5 /
 7200.00 3700.00 7300.00  .00000 7000.00  .00000     1      0       5 /

RSVD       2 TABLES    3 NODES IN EACH           FIELD   12:00 17 AUG 83
   7000.0  1.0000
   7990.0  1.0000
/
   7000.0  1.0000
   7400.0  1.0000
/

RPTRST
-- Restart File Output Control
--
'BASIC=2' 'FLOWS' 'POT' 'PRES' /


SUMMARY      ===========================================================

FOPR

WOPR
 /

FGPR

FWPR

FWIR

FWCT

FGOR

--RUNSUM

ALL

MSUMLINS

MSUMNEWT

SEPARATE

SCHEDULE     ===========================================================

DEBUG
   1 3   /

DRSDT
   1.0E20  /

RPTSCHED
  'PRES'  'SWAT'  'SGAS'  'RESTART=1'  'RS'  'WELLS=2'  'SUMMARY=2'
  'CPU=2' 'WELSPECS'   'NEWTON=2' /

NOECHO


ECHO

GRUPTREE
 'GRP1' 'FIELD' /
 'WGRP1' 'GRP1' /
 'WGRP2' 'GRP1' /
/

WELSPECS
 'PROD1' 'WGRP1' 1 5 7030 'OIL' 0.0  'STD'  'STOP'  /
 'PROD2' 'WGRP2' 1 5 7030 'OIL' 0.0  'STD'  'STOP'  /
 'WINJ1'  'WGRP1' 10 1 7030 'WAT' 0.0  'STD'  'STOP'   /
 'WINJ2'  'WGRP2' 10 1 7030 'WAT' 0.0  'STD'  'STOP'   /
/

COMPDAT

 'PROD1' 1 5 2 2   3*  0.2   3*  'X' /
 'PROD1' 2 5 2 2   3*  0.2   3*  'X' /
 'PROD1' 3 5 2 2   3*  0.2   3*  'X' /
 'PROD2' 4 5 2 2   3*  0.2   3*  'X' /
 'PROD2' 5 5 2 2   3*  0.2   3*  'X' /

 'WINJ1' 10 1  9 9   3*  0.2   3*  'X' /
 'WINJ1'   9 1  9 9   3*  0.2   3*  'X' /
 'WINJ1'   8 1  9 9   3*  0.2   3*  'X' /
 'WINJ2'   7 1  9 9   3*  0.2   3*  'X' /
 'WINJ2'   6 1  9 9   3*  0.2   3*  'X' /
/



UDQ
-- test 
--oil & liquid capacities at GEFAC = 0.8995
DEFINE WUOPRL (WOPR PROD1 - 150) * 0.90 /
DEFINE WULPRL (WLPR PROD1 - 200) * 0.90 /
DEFINE WUOPRU (WOPR PROD2 - 250) * 0.80 /
DEFINE WULPRU (WLPR PROD2 - 300) * 0.80 /
--DEFINE GUOPRU (GOPR GRP1 - 100) * 0.70 /
--DEFINE WUOPRL (WOPR PROD1 - 170) * 0.60 /
-- units
UNITS  WUOPRL SM3/DAY /
UNITS  WULPRL SM3/DAY /
UNITS  WUOPRU SM3/DAY /
--UNITS  GUOPRU SM3/DAY /
UNITS  WULPRU SM3/DAY /
--
/


--GCONPROD
--'GRP1' 'FLD'  -1  1* 1* 6000 'RATE' 'YES' 1* 'FORM' 7* /
--/

-- Well production rate targets/limits:
-- testing UDQs as production constrains
WCONPROD
-- name      status  ctrl   qo     qw  qg  ql	 qr bhp  thp  vfp  alq 
  'PROD1'     'OPEN'  'GRUP' WUOPRL  1*  1*  WULPRL 1* 60.0   / single wells
--  'PROD1'     'OPEN'  'GRUP' -2  1*  1*  -3 1* 60.0   / single wells
/


WCONPROD
-- name      status  ctrl   qo     qw  qg  ql	 qr bhp  thp  vfp  alq 
  'PROD2'     'OPEN'  'GRUP' WUOPRU  1*  1*  WULPRU 1* 60.0   / single wells
-- 'PROD1' 'OPEN' 'LRAT'  3*  1200  1*  2500  1*  /
-- 'PROD2' 'OPEN' 'LRAT'  3*    800  1*  2500  1*  /
 /

WCONINJE
 'WINJ1' 'WAT' 'OPEN' 'BHP'  1*  1200  3500  1*  /
 'WINJ2' 'WAT' 'OPEN' 'BHP'  1*    800  3500  1*  /
 /


TUNING
 /
 /
 /

TSTEP
 4
/


END

)~" };

	    return Opm::Parser{}.parseString(input);
	}

    Opm::UDQActive udq_active() {
      int update_count = 0;
      // construct record data for udq_active
      Opm::UDQParams params;
      Opm::UDQConfig conf(params);
      Opm::UDQActive udq_act;
      Opm::UDAValue uda1("WUOPRL");
      update_count += udq_act.update(conf, uda1, "PROD1", Opm::UDAControl::WCONPROD_ORAT);

      Opm::UDAValue uda2("WULPRL");
      update_count += udq_act.update(conf, uda2, "PROD1", Opm::UDAControl::WCONPROD_LRAT);
      Opm::UDAValue uda3("WUOPRU");
      update_count += udq_act.update(conf, uda3, "PROD2", Opm::UDAControl::WCONPROD_ORAT);
      Opm::UDAValue uda4("WULPRU");
      update_count += udq_act.update(conf, uda4, "PROD2", Opm::UDAControl::WCONPROD_LRAT);

      for (auto it = udq_act.begin(); it != udq_act.end(); it++) 
      {
	  auto ind = it->input_index;
	  auto udq_key = it->udq;
	  auto name = it->wgname;
	  auto ctrl_type = it->control;
      }
      return udq_act;
    }
}



//int main(int argc, char* argv[])
struct SimulationCase
{
    explicit SimulationCase(const Opm::Deck& deck)
	: es   { deck }
	, grid { deck }
	, sched{ deck, es }
    {}

    // Order requirement: 'es' must be declared/initialised before 'sched'.
    Opm::EclipseState es;
    Opm::EclipseGrid  grid;
    Opm::Schedule     sched;

};
    
BOOST_AUTO_TEST_SUITE(Aggregate_UDQ)


// test dimensions of multisegment data
BOOST_AUTO_TEST_CASE (Constructor)
{
    const auto simCase = SimulationCase{first_sim()};
        
    Opm::EclipseState es = simCase.es;
    Opm::Schedule     sched = simCase.sched;
    Opm::EclipseGrid  grid = simCase.grid;
    const auto& ioConfig = es.getIOConfig();
    const auto& restart = es.cfg().restart();
    */
    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{1};    
    
    std::string outputDir = "./";
    std::string baseName = "TEST_UDQRST";
    Opm::EclIO::OutputStream::Restart rstFile {
    Opm::EclIO::OutputStream::ResultSet { outputDir, baseName },
    rptStep,
    Opm::EclIO::OutputStream::Formatted { ioConfig.getFMTOUT() },
	  Opm::EclIO::OutputStream::Unified   { ioConfig.getUNIFOUT() }
        };
	
    double secs_elapsed = 3.1536E07;
    const auto ih = Opm::RestartIO::Helpers::createInteHead(es, grid, sched,
                                                secs_elapsed, rptStep, rptStep);
       
    const auto udqDims = Opm::RestartIO::Helpers::createUdqDims(sched, rptStep, ih);
    auto  udqData = Opm::RestartIO::Helpers::AggregateUDQData(udqDims);
    udqData.captureDeclaredUDQData(sched, rptStep, ih);
     
    rstFile.write("IUDQ", udqData.getIUDQ());
    rstFile.write("IUAD", udqData.getIUAD());
    rstFile.write("ZUDN", udqData.getZUDN());
    rstFile.write("ZUDL", udqData.getZUDL());
    
    }
    
BOOST_AUTO_TEST_SUITE_END()
