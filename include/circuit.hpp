#pragma once
#include <cstring>
#include <vector>
#include <cassert>
#include "logging.hpp"

namespace preproq {

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
        unsigned char pos:1 = 0;
        unsigned char neg:1 = 0;

        NodeChild head = 0;

        inline bool active() {
            return pos || neg;
        }
    };
    
    
    class Circuit {
        std::vector<GateVar> vars;
        std::vector<Literal> children;

        VarId firstGate = 0;

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
        }

        //adds a child to the previously specified gate
        void pushBackChild(Literal lit) {
            assert(children.size() > 0);
            children[children.size()-1] = lit << 1;
            children.push_back(0);
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
            return var(vid).head;
        }

        bool isEnd(NodeChild nc) {
            return get(nc) == 0;
        }

        NodeChild next(NodeChild child) {
            child++;
            Literal elem = children[child];
            if((elem & 1) > 0) {
                //Indirection
                return child + (elem >> 1); // relative reference
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

    };


}
