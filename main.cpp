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

    std::fstream input(argv[1], std::ios_base::in);

    bool cleansed = argc >= 3 && (std::string(argv[2]) == "--cleansed");
    
    ERROR_IF(!input.good(), "Input file " << argv[1] << " was not good!");

    Circuit circ;
    if(cleansed) {
        ERROR_IF(qcir::parse_cleansed(input, circ, argv[1]) != PREPROQ_OK, "Parser exited with errors!");
    }
    else {
        ERROR_IF(qcir::parse(input, circ, argv[1]) != PREPROQ_OK, "Parser exited with errors!");
    }

    PreProQ proq(circ);
    
    int result = proq.run();

    if(result == PREPROQ_OK)
        writeQcir(std::cout, circ);        
    
    return result;
}
