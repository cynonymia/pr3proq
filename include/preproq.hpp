#pragma once
#include <stack>
#include <vector>
#include "logging.hpp"
#include "circuit.hpp"


namespace preproq {

    using Sig = unsigned long long; 

    struct PreProQOptions {
        const char* target_file = nullptr;
        bool use_dagification:1 = false;
        bool qdimacs_out:1 = false;
    };
    
    class PreProQ {
        Circuit& circ;
        std::vector<char> tagbuffer;
        std::stack<VarId> tagged;
        
        NodeColor curColor = Red;

        void traceActive();
        bool localUnit();
        void dagify();

        inline void resetTagBuffer() {
            while(!tagged.empty()) {
                tagbuffer[tagged.top()] = 0;
                tagged.pop();
            }
        }

        //a good hash function https://nullprogram.com/blog/2018/07/31/
        unsigned prospector32(unsigned x)
        {
            x ^= x >> 15;
            x *= 0x2c1b3c6dU;
            x ^= x >> 12;
            x *= 0x297a2d39U;
            x ^= x >> 15;
            return x;
        }


        void cleanupUsage();

    public:
        PreProQOptions options;
        
        PreProQ(Circuit& circ) : circ(circ) {
            INF("Initializing");
            tagbuffer.resize(circ.varSize()+1);
            resetTagBuffer();
            traceActive();
        }

        int run();
    };

}
