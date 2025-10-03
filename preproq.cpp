#include <cassert>
#include <stack>
#include "preproq.hpp"

namespace preproq{

    void PreProQ::init() {
        INF("Initializing");
        std::stack<Literal> iteration;
        iteration.push(circ.root);
        if(circ.root < 0)
            circ.var(-circ.root).neg = 1;
        else
            circ.var(circ.root).pos = 1;

        while(iteration.size() > 0) {            
            Literal lit = iteration.top();
            PTR("Taking " << lit << " from iteration stack");
            VarId vid = VAR(lit);
            assert(circ.var(vid).active());
            iteration.pop();
            if(circ.var(vid).color == curColor) { //skip already visited
                PTR("Already processed, skip");
                continue;
            }
            circ.var(vid).color = curColor;
            for(NodeChild c = circ.begin(vid); !circ.isEnd(c); c = circ.next(c)) {
                Literal cl = circ.get(c);
                PTR("Child " << cl <<"(" << c <<")");
                circ.var(VAR(cl)).pos = circ.var(VAR(cl)).pos ||
                    ((cl > 0) && circ.var(vid).pos) || ((cl < 0) && circ.var(vid).neg);
                circ.var(VAR(cl)).neg = circ.var(VAR(cl)).neg ||
                    ((cl < 0) && circ.var(vid).pos) || ((cl > 0) && circ.var(vid).neg);
                if(circ.tseitin(VAR(cl))) {
                    PTR("Push " << cl << " to iteration stack");
                    iteration.push(cl);
                }
            }
        }
        curColor = SWITCH_COLOR(curColor);
    }


    #define PREFIX "[Iteration:" << iteration << "] " 
    int PreProQ::run() {        
        bool working = false;
        size_t iteration = 1;
        
        do {
            INF("Start Iteration " << iteration);
            for(VarId vid = circ.gateBegin(); vid != circ.gateEnd(); vid++) {
                assert(circ.tseitin(vid));

                if(!circ.var(vid).active()){
                    PDBG("Skipping inactive " << vid);
                    continue;
                }

                if(circ.var(vid).assignment != VA_None) {
                    PDBG("Skipping assigned " << vid);
                    continue;
                }
      
                PDBG("Working on " << vid);
                
                unsigned char gtype = circ.var(vid).gtype & 0b11;

                if(circ.isEnd(circ.begin(vid))) {
                    //Empty gate => assign
                    INF("Found empty gate " << vid << " - assigning!");
                    circ.var(vid).assignment = gtype == GT_And ? Assignment::VA_True : Assignment::VA_False;
                    continue;
                    //No additional working step => assignment will be propagated automatically as gates are ordered
                }
                resetTagBuffer();
                for(NodeChild c = circ.begin(vid); !circ.isEnd(c); c = circ.next(c)) {
                    Literal child = circ.get(c);                
                    PPTR("Checking " << child << " of " << vid);
                    
                    if(tagbuffer[VAR(child)] != 0) {
                        if((tagbuffer[VAR(child)] > 0) == (child > 0)) {
                            PINF("Eliminating duplicate " <<child << " in gate " << vid);
                            circ.deleteAt(c);
                            continue;
                        }
                        else {
                            PINF("Eliminating tautology/contradiction " << child <<
                                 ", " << -child << " in gate " << vid);
                            circ.var(vid).assignment = gtype == GT_And ? VA_False : VA_True;
                            break;
                        }                        
                    }
                    else {
                        tagbuffer[VAR(child)] = child < 0 ? -1 : 1;
                        tagged.push(VAR(child));
                    }

                    if(circ.var(VAR(child)).assignment != VA_None) {
                        //Constant propagation
                        Assignment a = static_cast<Assignment>(circ.var(VAR(child)).assignment);
                        if(child < 0)
                            a = static_cast<Assignment>(SWITCH_ASSIGNMENT(a));
                        if((a == VA_True) == (gtype == GT_Or)) {
                            PINF("Propagating constant from " << child << ": " << (a == VA_True ? "True" : "False"));
                            circ.var(vid).assignment = a;
                            break;   //skip out
                        }
                        else {
                            PINF("Deleting child falsified by constant propagation: " << child << " of " << vid);
                            circ.deleteAt(c);
                            continue;
                        }
                    }

                    if(circ.tseitin(VAR(child))) {                        
                        NodeChild grandc = circ.begin(VAR(child));

                        unsigned char cgtype = circ.var(VAR(child)).gtype;
                        
                        ERROR_IF(circ.isEnd(grandc), "Encountered non-assigned empty gate in children! Breach of bottom-up inconstraint!");

                        //Singularity elimination
                        if(circ.isEnd(circ.next(grandc))) {
                            Literal grandchild = circ.get(grandc);
                            PINF("Eliminating singularity " << child << " by replacing it with " << grandchild);
                            circ.set(c, child < 0 ? -grandchild : grandchild);
                            continue;
                        }
                        else if(cgtype == gtype && child > 0) {  //positive equal
                            PINF("Collapsing gate " << child << " into " << vid);
                            circ.set(c, circ.get(grandc));
                            grandc = circ.next(grandc);
                            while(!circ.isEnd(grandc)) {
                                circ.addChild(vid, circ.get(grandc));
                                grandc = circ.next(grandc);
                            }
                        }
                        else if(cgtype != gtype && child < 0) { //negative not equal
                            PINF("Collapsing gate " << child << " into " << vid);
                            circ.set(c, -circ.get(grandc));
                            grandc = circ.next(grandc);
                            while(!circ.isEnd(grandc)) {
                                circ.addChild(vid, -circ.get(grandc));
                                grandc = circ.next(grandc);
                            }
                        }
                    }
                }                
            }
            PDBG("End of iteration");
            iteration++;
        }
        while(working);
        int result = PREPROQ_OK;
        if(circ.var(VAR(circ.root)).assignment != VA_None) {
            result = (circ.root > 0) == (circ.var(VAR(circ.root)).assignment == VA_True) ? PREPROQ_SAT : PREPROQ_UNSAT;        
            INF("Preprocessor was able to solve the circuit with result " << result);
        }
        else {
            INF("Preprocessor finished without solving the circuit!");
        }
        return result;
    }
    
    #undef PREFIX
                             
}
