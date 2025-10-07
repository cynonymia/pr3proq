#pragma once
#include <stack>
#include <vector>
#include "logging.hpp"
#include "circuit.hpp"


namespace preproq {

    class PreProQ {
        Circuit& circ;
        std::vector<char> tagbuffer;
        std::stack<VarId> tagged;
        
        NodeColor curColor = Red;

        void traceActive();

        inline void resetTagBuffer() {
            while(!tagged.empty()) {
                tagbuffer[tagged.top()] = 0;
                tagged.pop();
            }
        }

        void cleanupUsage();

    public:
        PreProQ(Circuit& circ) : circ(circ) {
            INF("Initializing");
            tagbuffer.resize(circ.varSize()+1);
            resetTagBuffer();
            traceActive();
        }

        int run();
    };

}
