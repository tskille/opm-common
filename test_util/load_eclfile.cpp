#include <chrono>
#include <iomanip>
#include <iostream>
#include <tuple>
#include <getopt.h>
#include <numeric>

#include <opm/io/eclipse/EclFile.hpp>
//#include <opm/io/eclipse/ERst.hpp>
//#include <opm/io/eclipse/EclOutput.hpp>

using namespace Opm::EclIO;
using EclEntry = EclFile::EclEntry;


static void printHelp() {

    std::cout << "\nsimple program for testing EclFile performance with multithreading. program needs one argument which is filename to be loaded \n"
              << "\nIn addition, the program takes these options (which must be given before the arguments):\n\n"
              << "-h Print help and exit.\n"
              << "-t number of threads to be used, default no multithreading (equal to 1).\n\n";
}

int main(int argc, char **argv) {

    int c                          = 0;
    int max_num_threads            = 1;

    while ((c = getopt(argc, argv, "ht:")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        case 't':
            max_num_threads = atoi(optarg);
            break;
        default:
            return EXIT_FAILURE;
        }
    }

    int argOffset = optind;

    // start reading
    auto start = std::chrono::system_clock::now();
    std::string filename = argv[argOffset];

    //Opm::EclIO::EclFile file1 = Opm::EclIO::EclFile(filename);
    EclFile file1 = EclFile(filename);

    std::cout << "\nopen file: " << filename << std::endl;
    std::cout << "number of threads to be used: " << max_num_threads << std::endl;

    /*
    //file1.loadData();
    auto arrayList = file1.getList();
    int nArrays = arrayList.size() ;

    std::cout << "number of arrays: " << nArrays << std::endl;

    std::vector<int> arrayIndexList;
    arrayIndexList.resize(nArrays,0);
    std::iota(arrayIndexList.begin(), arrayIndexList.end(), 0);

    //file1.loadData();  // init file -> 5.16791 seconds
    file1.loadData(arrayIndexList, max_num_threads);  //  init file -> 5.12199 seconds
    */

    file1.loadData({}, max_num_threads);  //  init file -> 5.12199 seconds

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    std::cout << "\n\nruntime loading : " << argv[argOffset] << ": " << elapsed_seconds.count() << " seconds\n" << std::endl;

/*
    EclFile file1(filename);
    bool formattedOutput = file1.formattedInput() ? false : true;

    int p = filename.find_last_of(".");
    int l = filename.length();

    std::string rootN = filename.substr(0,p);
    std::string extension = filename.substr(p,l-p);
    std::string resFile;

    if (listProperties) {

        if (extension==".UNRST") {

            ERst rst1(filename);
            rst1.loadData("INTEHEAD");

            std::vector<int> reportStepList=rst1.listOfReportStepNumbers();

            for (auto seqn : reportStepList) {

                std::vector<int> inteh = rst1.getRst<int>("INTEHEAD", seqn, 0);

                std::cout << "Report step number: "
                          << std::setfill(' ') << std::setw(4) << seqn << "   Date: " << inteh[66] << "/"
                          << std::setfill('0') << std::setw(2) << inteh[65] << "/"
                          << std::setfill('0') << std::setw(2) << inteh[64] << std::endl;
            }

            std::cout << std::endl;

        } else {
            std::cout << "\n!ERROR, option -l only only available for unified restart files (*.UNRST) " << std::endl;
            exit(1);
        }

        return 0;
    }

    std::map<std::string, std::string> to_formatted = {{".EGRID", ".FEGRID"}, {".INIT", ".FINIT"}, {".SMSPEC", ".FSMSPEC"}, 
        {".UNSMRY", ".FUNSMRY"}, {".UNRST", ".FUNRST"}, {".RFT", ".FRFT"}};

    std::map<std::string, std::string> to_binary = {{".FEGRID", ".EGRID"}, {".FINIT", ".INIT"}, {".FSMSPEC", ".SMSPEC"}, 
        {".FUNSMRY", ".UNSMRY"}, {".FUNRST", ".UNRST"}, {".FRFT", ".RFT"}};
        
    if (formattedOutput) {
        
        auto search = to_formatted.find(extension);
    
        if (search != to_formatted.end()){
            resFile = rootN + search->second;     
        } else if (extension.substr(1,1)=="X"){
            resFile = rootN + ".F" + extension.substr(2);
        } else if (extension.substr(1,1)=="S"){
            resFile = rootN + ".A" + extension.substr(2);
        } else {
            std::cout << "\n!ERROR, unknown file type for input file '" << rootN + extension << "'\n" << std::endl;
            exit(1);
        }
    } else {
        
        auto search = to_binary.find(extension);

        if (search != to_binary.end()){
            resFile = rootN + search->second;     
        } else if (extension.substr(1,1)=="F"){
            resFile = rootN + ".X" + extension.substr(2);
        } else if (extension.substr(1,1)=="A"){
            resFile = rootN + ".S" + extension.substr(2);
        } else {
            std::cout << "\n!ERROR, unknown file type for input file '" << rootN + extension << "'\n" << std::endl;
            exit(1);
        }
    }

    std::cout << "\033[1;31m" << "\nconverting  " << argv[argOffset] << " -> " << resFile << "\033[0m\n" << std::endl;

    EclOutput outFile(resFile, formattedOutput);

    if (specificReportStepNumber) {

        if (extension!=".UNRST") {
            std::cout << "\n!ERROR, option -r only can only be used with unified restart files (*.UNRST) " << std::endl;
            exit(1);
        }

        ERst rst1(filename);

        if (!rst1.hasReportStepNumber(reportStepNumber)) {
            std::cout << "\n!ERROR, selected unified restart file doesn't have report step number " << reportStepNumber << "\n" << std::endl;
            exit(1);
        }

        rst1.loadReportStepNumber(reportStepNumber);

        auto arrayList = rst1.listOfRstArrays(reportStepNumber);

        writeArrayList(arrayList, rst1, reportStepNumber, outFile);

    } else {

        file1.loadData();
        auto arrayList = file1.getList();

        writeArrayList(arrayList, file1, outFile);
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    std::cout << "runtime  : " << argv[argOffset] << ": " << elapsed_seconds.count() << " seconds\n" << std::endl;
*/
    return 0;
}
