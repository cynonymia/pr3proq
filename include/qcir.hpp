#pragma once

#include <istream>
#include "circuit.hpp"

namespace preproq::qcir {
    int parse(FILE* input, Circuit& circ, std::string target = "<internal>");

}
