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
    bool use_pthread               = false;

    while ((c = getopt(argc, argv, "ht:p")) != -1) {
        switch (c) {
        case 'h':
            printHelp();
            return 0;
        case 't':
            max_num_threads = atoi(optarg);
            break;
        case 'p':
            use_pthread=true;
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

    if (use_pthread){
        std::cout << "using pthread" << std::endl;
        file1.loadData2({}, max_num_threads);
    } else {
        std::cout << "using OpenMP" << std::endl;
        file1.loadData({}, max_num_threads);  //  using OpenMP
    }

    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;

    std::cout << "\n\nruntime loading : " << argv[argOffset] << ": " << elapsed_seconds.count() << " seconds\n" << std::endl;


    return 0;
}
