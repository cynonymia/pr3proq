#pragma once
#include <cstring>
#include <iomanip>
#include <vector>
#include <cassert>
#include "logging.hpp"

namespace preproq {

#define TRUE_QCIR "#QCIR-14\nexists(1)\noutput(2)\n2 = or(1, -1)\n"
#define FALSE_QCIR "#QCIR-14\nexists(1)\noutput(2)\n2 = and(1, -1)\n"

    
#define VAR(x) abs(x)
    
    using VarId = unsigned;
    using Literal = int;

    enum GType: unsigned char {
        GT_Or = 0,
        GT_And = 1
    };
    
    enum NodeColor : unsigned char {
        Black = 0,
        Red = 1
    };

    #define SWITCH_COLOR(x) (x == Red ? Black : Red)

    enum QType : unsigned char {
        Free = 0,
        Exists = 1,
        Forall = 2,
        Tseitin = 3
    };

    enum Assignment : unsigned char  {
        VA_None = 0,
        VA_True = 1,
        VA_False = 2
    };

    #define SWITCH_ASSIGNMENT(x) (x ^ 0b11)

    using NodeChild = size_t;    
    
    struct GateVar {        
        unsigned char qtype:2 = Free;
        unsigned char color:1 = Black;
        unsigned char gtype:1 = GT_Or;
        unsigned char assignment:2 = VA_None;
        unsigned char localMark:1 = 0;  //if 1, the current assignment is only local
        unsigned char pos:1 = 0;
        unsigned char neg:1 = 0;
        unsigned char dag:1 = 0;    //if 1, more than 1 reference is made to this Variable

        NodeChild head = 0;

        inline bool active() {
            return pos || neg;
        }
    };
    
    
    class Circuit {
        std::vector<GateVar> vars;
        std::vector<Literal> children;

        VarId firstGate = 0;

        VarId currentLast = 0;

        inline bool isIndirection(NodeChild nid) {
            assert(nid < children.size());
            return (children[nid] & 1) > 0;
        }
    public:

        Literal root = 0;
        
        Circuit() {
            vars.emplace_back(); //pad first element
            children.push_back(0); //pad first element as invalid iterator
        }

        inline size_t varSize() {
            return vars.size()-1;
        }

        inline GateVar& var(VarId vid) {
            assert(vid > 0 && vid < vars.size());
            return vars[vid];
        }

        inline bool tseitin(VarId vid) {
            return var(vid).qtype == Tseitin;
        }
                
        void addVar(VarId vid, QType qtype) {
            if(varSize() <= vid) 
                vars.resize(vid+1, GateVar());            
            vars[vid].qtype = qtype;
        }

        void addGate(VarId vid, GType gt) {
            if(varSize() <= vid) 
                vars.resize(vid+1, GateVar());
            if(firstGate == 0)
                firstGate =  vid;
            vars[vid].qtype = Tseitin;
            vars[vid].gtype = gt;
            vars[vid].head = children.size();
            children.push_back(0);  //initialize empty children list
            currentLast = vid;
        }

        //adds a child to the previously specified gate
        void pushBackChild(Literal lit) {
            assert(children.size() > 0);
            children[children.size()-1] = lit << 1;
            children.push_back(0);
        }

        void addChild(VarId vid, Literal lit) {
            if(currentLast == vid){
                pushBackChild(lit);
                return;
            }
            NodeChild nc = var(vid).head;            
            do {
                if(isEnd(nc)) {
                    unsigned target = children.size();
                    children.push_back(lit << 1);
                    children.push_back(0);
                    children[nc] = ((target - nc) << 1) + 1;    //set indirection
                    currentLast = vid;
                    return;
                }
                
                if(isIndirection(nc)){
                    if (get(nc) == 1)  {
                        children[nc] = lit << 1;    //fill in for deleted element
                        return;
                    }
                    else
                        nc = nc + get(nc); // resolve one relative ref
                }
                else
                    nc++;
            }while(nc < children.size());
            assert(false);  //should never get here
        }

        inline VarId gateBegin() {
            return firstGate;
        }

        inline VarId gateEnd() {
            return vars.size();            
        }

        inline VarId varBegin() {
            return 1;
        }

        inline VarId varEnd() {
            return vars.size();
        }

        Literal get(NodeChild child) {
            assert(child < children.size());
            return children[child] >> 1;
        }
        
        NodeChild begin(VarId vid) {
            NodeChild nc = var(vid).head;
            while(isIndirection(nc)) {
                nc = nc + (children[nc] >> 1); // relative reference
            }
            return nc;
        }

        bool isEnd(NodeChild nc) {
            return get(nc) == 0;
        }

        NodeChild next(NodeChild child) {
            child++;
            Literal elem = children[child];
            while(isIndirection(child)) {
                //Indirection
                child = child + (elem >> 1); // relative reference
                elem = children[child];
            }
            return child;
        }

        inline void deleteAt(NodeChild child) {
            assert(!isIndirection(child));
            children[child] = 0b11; // go one step from current position => skip this element, effective deletion
        }
        
        inline void set(NodeChild child, Literal lit) {
            assert(!isIndirection(child));
            children[child] = lit << 1;
        }

        inline size_t calculateChildrenCount(VarId vid) {
            if(!tseitin(vid)) return 0;
            NodeChild n = var(vid).head;
            size_t count = 0;
            while(!isEnd(n)) {
                count++;
                n = next(n);
            }
            return count;
        }

#ifndef NDEBUG
        void memDump(std::ostream& out) {
            size_t cnt = 0;
            for(VarId vid = varBegin(); vid != varEnd(); vid++) {
                if(cnt % 10 == 0 && cnt != 0) out << std::endl;
                out << vid << "[" << var(vid).head << "] ";
                cnt++;
            }
            out << std::endl << std::endl;

            cnt = 0;
            out.fill('0');
            for(size_t i = 0; i < children.size(); i++) {
                if(cnt % 10 == 0) {
                    if(cnt != 0)
                        out << std::endl;
                    out << std::setw(6) << (cnt / 10)<< std::setw(0) << "    ";
                }
                out << children[i] << " ";
                cnt++;
            }
        }
#endif
    };

}
