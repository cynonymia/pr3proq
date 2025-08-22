#pragma once
#include <vector>
#include "logging.hpp"
#include "circuit.hpp"


namespace preproq {

    class PreProQ {
        Circuit& circ;
        std::vector<char> tagbuffer;

        NodeColor curColor = Red;

        void init();

        inline void resetTagBuffer() {
            memset(&tagbuffer[0], 0, tagbuffer.size() * sizeof(char));
        }

    public:
        PreProQ(Circuit& circ) : circ(circ) {
            tagbuffer.resize(circ.varSize()+1);
            resetTagBuffer();
            init();
        }

        int run();
    };

}
