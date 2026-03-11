#include <cstdio>
#include <fstream>
#include "logging.hpp"
#include "circuit.hpp"
#include "qcir.hpp"
#include "preproq.hpp"
#include "writer.hpp"

using namespace preproq;

int main(int argc, char** argv){    
    //global_verbose = VERBOSE_POINTER_LEVEL;
    if(argc < 2) {
        INF("No input specified! Shutting down...");
        return PREPROQ_OK;
    }

    FILE* fp = fopen(argv[1], "r");
    
    ERROR_IF(fp == nullptr, "Input file " << argv[1] << " was not good!");

    Circuit circ;

    ERROR_IF(qcir::parse(fp, circ, argv[1]) != PREPROQ_OK, "Parser exited with errors!");

    fclose(fp);
   
    PreProQ proq(circ);
    int result = proq.run();

    if(result == PREPROQ_OK)
        writeQcir(std::cout, circ);         

    return result;
}
