#include <fstream>
#include "logging.hpp"
#include "circuit.hpp"
#include "qcir.hpp"
#include "preproq.hpp"
#include "writer.hpp"

using namespace preproq;

int main(int argc, char** argv){
    //global_verbose = VERBOSE_DEBUG;
    if(argc < 2) {
        INF("No input specified! Shutting down...");
        return PREPROQ_OK;
    }

    std::fstream input(argv[1], std::ios_base::in);

    ERROR_IF(!input.good(), "Input file " << argv[1] << " was not good!");

    Circuit circ;
    ERROR_IF(qcir::parse_cleansed(input, circ) != PREPROQ_OK, "Parser exited with errors!");

    PreProQ proq(circ);
    
    int result = proq.run();

    if(result == PREPROQ_OK)
        writeQcir(std::cout, circ);        
    
    return result;
}
