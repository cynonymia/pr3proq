#pragma once

#include <istream>
#include "circuit.hpp"

namespace preproq::qcir {
    int parse(std::istream& input, Circuit& circ, std::string target = "<internal>");
    int parse_cleansed(std::istream& input, Circuit& circ, std::string target = "<internal>");
}
