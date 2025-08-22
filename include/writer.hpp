#pragma once
#include <ostream>
#include "circuit.hpp"

namespace preproq {
    void writeQcir(std::ostream& out, Circuit& circ);
    void writeQdimacs(std::ostream& out, Circuit& circ);
}   