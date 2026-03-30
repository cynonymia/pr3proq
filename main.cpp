#include <cstdio>
#include <fstream>
#include "globals.hpp"
#include "logging.hpp"
#include "circuit.hpp"
#include "qcir.hpp"
#include "preproq.hpp"
#include "writer.hpp"

using namespace preproq;

#define ARGS_HELP 0xF1
#define CSTR_EQUAL(x, y) (!strcmp(x,y))

static void printHelp() {
    WRITELN("pr3proq v3.0 (DEV) - a preprocessor for Q-Circuits");
    WRITELN("usage: preproq FORMULA [OPTIONS...]");
    WRITELN("");
    WRITELN("FORMULA expects a Circuit formula QCIR format");
    WRITELN("The following values for OPTIONS are valid");
    WRITELN("  -h, --help           displays this text");
    WRITELN("  --quiet              does not print any logging messages, not even errors");
    WRITELN("  --info               increases verbosity level to \"info\"");
#ifndef NDEBUG        
    WRITELN("  --debug              increases verbosity level to \"debug\"");
    WRITELN("  --plevel             increases verbosity level to \"pointer level\"");
#endif
    WRITELN("  --dagify             exploiting DAG property by re-evaluating gate references");
}

static int parseArgs(int argc, const char** argv, PreProQOptions& options) {
    for(int i =1; i < argc; i++) {
        if(CSTR_EQUAL(argv[i], "-h") || CSTR_EQUAL(argv[i], "--help")) {            
            return ARGS_HELP;
        }
        else if(CSTR_EQUAL(argv[i], "--quiet")) {
            global_verbose = VERBOSE_SILENT;
        }
        else if(CSTR_EQUAL(argv[i], "--info")) {
            global_verbose = VERBOSE_INFO;
        }
#ifndef NDEBUG     
        else if(CSTR_EQUAL(argv[i], "--debug")) {
            global_verbose = VERBOSE_DEBUG;
        }
        else if(CSTR_EQUAL(argv[i], "--plevel")) {
            global_verbose = VERBOSE_POINTER_LEVEL;
        }
#endif
        else if(CSTR_EQUAL(argv[i], "--dagify")) {
            options.use_dagification = true;
        }        
        else if(!options.target_file){
            if(CSTR_EQUAL(argv[i], "-"))
                options.target_file = "/dev/stdin";
            else
                options.target_file = argv[i];
        }
        else {
            ERR("[ArgumentParsing] Unknown option " << argv[i] << "!");
            return PREPROQ_OK;
        }        
    }
    return PREPROQ_OK;
}


int main(int argc, const char** argv){    

    PreProQOptions opt;
    int argparseRes = parseArgs(argc, argv, opt);    
    if(argparseRes == ARGS_HELP) {
        printHelp();
        return PREPROQ_OK;
    }
    else if(argparseRes != PREPROQ_OK)
        return argparseRes;  

    
    if(opt.target_file == nullptr) {
        DBG("No input file specified, using stdin");
        opt.target_file = "/dev/stdin";
    }
    
    FILE* fp = fopen(opt.target_file, "r");
    
    ERROR_IF(fp == nullptr, "Input file " << opt.target_file << " could not be opened!");

    Circuit circ;

    ERROR_IF(qcir::parse(fp, circ, argv[1]) != PREPROQ_OK, "Parser exited with errors!");

    fclose(fp);
   
    PreProQ proq(circ);
    proq.options = opt;
    int result = proq.run();

    if(result == PREPROQ_OK)
        writeQcir(std::cout, circ);         

    return result;
}
