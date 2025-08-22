#pragma once

#include <istream>
#include "circuit.hpp"

namespace preproq::qcir {    
    int parse_cleansed(std::istream& input, Circuit& circ);
}
