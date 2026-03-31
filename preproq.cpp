#include <cassert>
#include <fstream>
#include <map>
#include <stack>
#include "preproq.hpp"
#include "circuit.hpp"
#include "logging.hpp"

namespace preproq{

    void PreProQ::traceActive() {
        DBG("Tracing active gates");
        if(circ.root < 0)
            circ.var(-circ.root).neg = 1;
        else
            circ.var(circ.root).pos = 1;

        for(VarId vid = VAR(circ.root); vid >= circ.gateBegin(); vid--) {
            if(!circ.var(vid).active())
                continue;

            for(NodeChild c = circ.begin(vid); !circ.isEnd(c); c = circ.next(c)) {
                Literal lit = circ.get(c);
                GateVar& var = circ.var(VAR(lit));
                PTR("Child " << lit <<"(" << c <<")");
                
                var.dag = var.pos || var.neg; //if already set, multiple references are made to this variable
                var.pos = var.pos || ((lit > 0) && circ.var(vid).pos) || ((lit < 0) && circ.var(vid).neg);                
                var.neg = var.neg || ((lit < 0) && circ.var(vid).pos) || ((lit > 0) && circ.var(vid).neg);            

            }            
        }
    }

    void PreProQ::cleanupUsage() {
        DBG("Reset flags on variables to retrace");
        for(VarId vid = circ.varBegin(); vid != circ.varEnd(); vid++) {
            circ.var(vid).neg = 0;
            circ.var(vid).pos = 0;
            circ.var(vid).dag = 0;
        }        
        
        traceActive();
    }    

#define PREFIX "[LocalUnitRed] "
    bool PreProQ::localUnit() {
        PDBG("Start");
        bool reduced = false;
        std::stack<VarId> working;  //for gates of the current derivation tree
        std::stack<VarId> stale;    //for gates that need a hard reset
        std::stack<VarId> assigned; //for the assignments of the local units
        std::stack<int> localCount; //count of local knowledge variables
        
        working.push(VAR(circ.root));   //start with root

        while(!working.empty() || !stale.empty()) {
            PPTR("Outer iteration started with working:" << working.size() << ", stale:" << stale.size());
            if(working.empty()) {   //transfer 
                working.push(stale.top());
                PPTR("Transfer " << stale.top() << " to working");
                stale.pop();
            }

            //process working stack
            do {
                PPTR("Inner iteration started");
                assert(circ.tseitin(working.top()));
                VarId vid = working.top();
                GType gtype = static_cast<GType>(circ.var(vid).gtype);
                working.pop();

                if(circ.var(vid).color == curColor){ //already visited
                    PPTR("Skip already visited " << vid);
                    continue;
                }
                circ.var(vid).color = curColor; //mark visited

                working.push(0);    //mark scope

                int assignments = 0;
                for(NodeChild c = circ.begin(vid); !circ.isEnd(c); c = circ.next(c)) {
                    Literal lit = circ.get(c);
                    PPTR("Processing " << lit << " of " << vid);
                                        
                    if(!circ.tseitin(VAR(lit))) {   //push knowledge

                        if(circ.var(VAR(lit)).assignment != VA_None) {  //apply knowledge
                            reduced = true;
                            Assignment a = static_cast<Assignment>(circ.var(VAR(lit)).assignment);
                            if(lit < 0)
                                a = static_cast<Assignment>(SWITCH_ASSIGNMENT(a));
                                                       
                            if((a == VA_True) == (gtype == GT_Or)) {
                                PINF("Propagating Constant from " << lit << ": " << (a == VA_True ? "True" : "False") <<
                                     " in gate " << vid);
                                INTR("localUnitProp " << vid << " " << lit << " " << (a == VA_True ? 1 : 0));
                                circ.var(vid).assignment = a;
                                break;  //skip out, this gate is finished
                            }
                            else {
                                PINF("Deleting child falsified by local unit: " << lit << " of " << vid);
                                INTR("localUnitDel " << vid << " " << lit);
                                circ.deleteAt(c);
                            }
                        }
                        else {
                            circ.var(VAR(lit)).assignment = (gtype == GType::GT_And) == (lit > 0) ? VA_True : VA_False;
                            circ.var(VAR(lit)).localMark = 1;   //mark as local
                            assigned.push(VAR(lit));
                            PPTR("Found unassigned Local Unit " << lit << ", assigning " <<
                                 (circ.var(VAR(lit)).assignment == VA_False ? "False" : "True"));
                            INTR("localUnit " << vid << " " << abs(lit)*(circ.var(VAR(lit)).assignment == VA_False ? -1 : 1));
                            assignments++;
                        }
                    }
                    else {
                        if(circ.var(VAR(lit)).assignment != VA_None)    //ignore assigned gates
                            continue;                        
                        if(circ.var(VAR(lit)).dag){ //as referenced multiple times, need a hard reset
                            PPTR("Pushing " << VAR(lit) << " to stale");
                            stale.push(VAR(lit));                            
                        }
                        else {
                            PPTR("Pushing " << VAR(lit) << " to working");
                            working.push(VAR(lit));                            
                        }
                    }
                }

                localCount.push(assignments);

                //backtrack
                while(!working.empty() && working.top() == 0) {
                    int remcnt = localCount.top();
                    PDBG("Backtracking with forgetting " << remcnt << " elements");
                    localCount.pop();
                    while(remcnt--) {
                        INTR("localUnitUnlearn " << (Literal) (assigned.top() * (circ.var(assigned.top()).assignment == VA_True ? 1 : -1)));
                        circ.var(assigned.top()).assignment = VA_None;
                        circ.var(assigned.top()).localMark = 0;
                        PPTR("Forgetting " << assigned.top());
                        assigned.pop();                   
                    }
                    working.pop();
                }
                                
            } while(!working.empty());
            
            //reset assignments, if any left
            while(!assigned.empty()) {
                circ.var(assigned.top()).assignment = VA_None;
                circ.var(assigned.top()).localMark = 0;
                assigned.pop();
            }
            while(!localCount.empty())
                localCount.pop();            
        }
        curColor = SWITCH_COLOR(curColor);
        return reduced;        
    }
    #undef PREFIX


    void PreProQ::dagify() {
        DBG("Start with DAG-ification");
        std::map<Sig, VarId> hashes;
        std::map<VarId, VarId> aliases;

        std::stack<VarId> visited;
        
        for(VarId vid = circ.gateBegin(); vid != circ.gateEnd(); vid++) {
            if(!circ.var(vid).active())
                continue;           
            
            if(circ.var(vid).assignment != VA_None)
                continue;
            
            Sig cur = 0;
            for(NodeChild c = circ.begin(vid); !circ.isEnd(c); c = circ.next(c)) {
                Literal lit = circ.get(c);
                auto it = aliases.find(VAR(lit));
                if(it != aliases.end()) {
                    circ.set(c, lit < 0 ? -it->second : it->second);
                    INF("Replacing child " << lit << " in " << vid << " with alias " << circ.get(c));
                }
                
                cur += prospector32(circ.get(c));
            }
            auto it = hashes.find(cur);

            if(it != hashes.end() && (circ.var(vid).gtype == circ.var(it->second).gtype)) {
                VarId other = it->second;
                DBG("Found candidate: " << vid << " == " << other);

                assert(visited.empty());
                
                int cnt = 0;
                for(NodeChild c = circ.begin(vid); circ.isEnd(c); c = circ.next(c)) {
                    Literal lit = circ.get(c);
                    assert(!circ.var(VAR(lit)).localMark);  //sanity check
                    circ.var(VAR(lit)).localMark = 1;
                    visited.push(VAR(lit));
                    cnt++;
                }

                for(NodeChild c = circ.begin(other); circ.isEnd(c); c = circ.next(c)) {
                    Literal lit = circ.get(c);
                    if(!circ.var(VAR(lit)).localMark) {
                        DBG("Candidate failed with witness " << lit);
                        break;
                    }
                    circ.var(VAR(lit)).localMark = 0;
                    cnt--;                        
                }
                while(!visited.empty()) {
                    if(circ.var(visited.top()).localMark) {
                        cnt = 1;    //set s.t. it fails
                        circ.var(visited.top()).localMark = 0;
                    }
                }
                if(cnt == 0)
                    aliases[vid] = other;
                else {
                    DBG("Hit with " << cur << " on two candidates " << vid << ", " << other);
                }
            }
            else {
                hashes[cur] = vid;
                PTR("Registering " << vid << " = " << cur);
            }
        }
        
    }

    
    #define PREFIX "[Iteration:" << iteration << "] " 
    int PreProQ::run() {        
        bool working = false;
        size_t iteration = 1;
#ifdef DBG_MEM_DMP        
        std::fstream f("0.memdmp", std::ios_base::out);
        circ.memDump(f);
        f.close();
#endif        
        do {
            working = false;
            INF("Start Iteration " << iteration);

            
            //Pure Literal elimination
            for(VarId vid = circ.varBegin(); vid < circ.varEnd(); vid++) {                
                if(circ.tseitin(vid)) break;
                
                if(circ.var(vid).assignment != VA_None) //Ignore already assigned
                    continue;
                
                if(circ.var(vid).neg != circ.var(vid).pos) {
                    Assignment a = circ.var(vid).qtype == QType::Forall ? VA_False : VA_True;
                    if(circ.var(vid).neg)
                        a = static_cast<Assignment>(SWITCH_ASSIGNMENT(a));
                    PINF("Pure literal reduction: assigning " << vid << " = " << (a == VA_False ? "False" : "True"));
                    INTR("pureLiteral " << " " << vid << " " << (a == VA_False ? 0 : 1));
                    circ.var(vid).assignment = a;
                }
            }
            
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

                resetTagBuffer();

                for(NodeChild c = circ.begin(vid); !circ.isEnd(c); c = circ.next(c)) {
                    Literal child = circ.get(c);
                    PPTR("Checking " << child << " of " << vid);
                    
                    if(tagbuffer[VAR(child)] != 0) {
                        if((tagbuffer[VAR(child)] > 0) == (child > 0)) {
                            PINF("Eliminating duplicate " <<child << " in gate " << vid);
                            circ.deleteAt(c);
                            INTR("duplicate " << vid << " " << child);
                            continue;
                        }
                        else {
                            PINF("Eliminating tautology/contradiction " << child <<
                                 ", " << -child << " in gate " << vid);
                            circ.var(vid).assignment = gtype == GT_And ? VA_False : VA_True;
                            INTR("tautology " << vid << " " << child << " " << (GT_And ? 0 : 1));
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
                            PINF("Propagating constant from " << child << " in " << vid <<  ": " << (a == VA_True ? "True" : "False"));
                            INTR("constProp " << vid << " " << child << " " << (a == VA_True ? 1 : 0));
                            circ.var(vid).assignment = a;
                            break;   //skip out
                        }
                        else {
                            PINF("Deleting child falsified by constant propagation: " << child << " of " << vid);
                            INTR("constDel " << vid << " " << child);
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
                            INTR("singularity " << vid << " " << child << " " << (child < 0 ? -grandchild : grandchild));
                            circ.set(c, child < 0 ? -grandchild : grandchild);
                            working = true;
                        }
                        //Gate Collapse
                        else if(cgtype == gtype && child > 0) {  //positive equal
                            PINF("Collapsing gate " << child << " into " << vid);
                            INTR("gateCollapse " << vid << " " << child);
                            circ.set(c, circ.get(grandc));
                            grandc = circ.next(grandc);
                            while(!circ.isEnd(grandc)) {
                                circ.addChild(vid, circ.get(grandc));
                                grandc = circ.next(grandc);
                            }
                            working = true;
                        }
                        else if(cgtype != gtype && child < 0) { //negative not equal
                            PINF("Collapsing gate " << child << " into " << vid);
                            INTR("gateCollapse " << vid << " " << child);
                            circ.set(c, -circ.get(grandc));
                            grandc = circ.next(grandc);
                            while(!circ.isEnd(grandc)) {
                                circ.addChild(vid, -circ.get(grandc));
                                grandc = circ.next(grandc);
                            }
                            working = true;
                        }
                    }
                }
                //After eliminiation, check if empty
                if(circ.isEnd(circ.begin(vid))) {
                    //Empty gate => assign
                    PINF("Found empty gate " << vid << " - assigning!");
                    circ.var(vid).assignment = gtype == GT_And ? Assignment::VA_True : Assignment::VA_False;
                    INTR("implicitConstant " << vid << " " << (gtype == GT_And ? 1 : 0));
                    continue;
                    //No additional working step => assignment will be propagated automatically as gates are ordered
                }
            }
            cleanupUsage();
            if(circ.var(VAR(circ.root)).assignment != VA_None){    //Assignment already known
                PINF("Root was assigned, circuit result is known!");
                break;
            }
            
            working = localUnit() || working;  //finally, apply local units which will determine whether a next iteration is needed
#ifdef DBG_MEM_DMP
            std::fstream f(std::to_string(iteration) + ".memdmp", std::ios_base::out);
            circ.memDump(f);
#endif            
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
            if(options.use_dagification)
                dagify();
            INF("Preprocessor finished without solving the circuit!");
        }        
        return result;
    }
    
    #undef PREFIX
                             
}
