#include "writer.hpp"
#include "circuit.hpp"
#include "preproq.hpp"

namespace preproq {

    void writeQcir(std::ostream& out, Circuit& circ) {
        out << "#QCIR-14" << std::endl;
        QType curq = Free;
        for(VarId vid = circ.varBegin(); vid < circ.varEnd(); vid++) {

            if(!circ.var(vid).active()) 
                continue;   //skip zombies
                            
            if(circ.var(vid).assignment != VA_None) 
                continue; //skip assigned
            

            QType q  = static_cast<QType>(circ.var(vid).qtype);

            if(q != Tseitin) {
                if(curq != q) {
                    if(curq != Free) out << ")" << std::endl;
                    out << (q == Forall ? "forall" : "exists") << "(";
                    curq = q;
                }
                else
                    out << ", ";

                out << vid;
            }
            else {
                if(curq != Tseitin) {
                    out << ")" << std::endl << "output(" << circ.root << ")" << std::endl;
                    curq = Tseitin;
                }
                out << vid << " = " << (circ.var(vid).gtype == GT_And ? "and": "or") << "(";
                NodeChild nc = circ.begin(vid);
                bool first = true;
                while(!circ.isEnd(nc)) {
                    if(first) first = false;
                    else out << ", ";
                    out << circ.get(nc);
                    nc = circ.next(nc);
                }
                out << ")" << std::endl;
            }
        }
    }

    static const char* qtype_to_str(QType qt) {
        switch(qt) {
        case preproq::QType::Exists: return "e";
        case preproq::QType::Forall: return "a";
        case preproq::QType::Tseitin: return "e";
        default: return "?";
        }
    }

    static size_t precalculate(Circuit& circ) {
        size_t count = 0;
        for(VarId vid = circ.gateBegin(); vid < circ.gateEnd(); vid++) {
            GType gt = static_cast<GType>(circ.var(vid).gtype);

            if(!circ.var(vid).active())
                continue;

            if(gt == GT_And) {
                if(circ.var(vid).pos)
                    count += circ.calculateChildrenCount(vid);
                if(circ.var(vid).neg)
                    count += 1;
            }
            else if(gt == GT_Or) {
                if(circ.var(vid).pos)
                    count += 1;
                if(circ.var(vid).neg)
                    count += circ.calculateChildrenCount(vid);
            }            
        }
        count += 1; //output literal
        return count;
    }

    void writeQdimacs(std::ostream& out, Circuit& circ) {
        out << "p cnf " << circ.varEnd()-1 << " " << precalculate(circ);

        QType qcur = QType::Free;
        //Prefix
        for(VarId vid = circ.varBegin(); vid < circ.varEnd(); vid++) {
            if(circ.var(vid).qtype != qcur) {               
                if(qcur != QType::Free)
                    out << " 0";
                qcur = static_cast<QType>(circ.var(vid).qtype);
                out << std::endl;
                out << qtype_to_str(qcur);
            }
            out << " " << vid;
        }
        if (qcur != QType::Free)
            out << " 0" << std::endl;

        std::vector<Literal> clause;
        //Matrix
        for(VarId vid = circ.gateBegin(); vid < circ.gateEnd(); vid++) {
            GType gt = static_cast<GType>(circ.var(vid).gtype);

            if(!circ.var(vid).active())
                continue;
                
            clause.clear();
            for(NodeChild c = circ.begin(vid); !circ.isEnd(c); c = circ.next(c))
                clause.push_back(circ.get(c));           
            
            if(gt == GT_And) {
                if(circ.var(vid).pos) {
                    for(Literal c : clause) {
                        out << -static_cast<int>(vid) << " " << c << " 0" << std::endl;
                    }
                }
                if(circ.var(vid).neg) {
                    out << vid << " ";
                    for(Literal c : clause) {
                        out << -c << " ";
                    }
                    out << "0" << std::endl;                    
                }
            }
            else if(gt == GT_Or) {
                if(circ.var(vid).pos){
                    out << -static_cast<int>(vid) << " ";
                    for(Literal c : clause) {
                        out << c << " ";
                    }
                    out << "0" << std::endl;                    
                }
                if(circ.var(vid).neg){
                    for(Literal c : clause) {
                        out << vid << " " << -c << " 0" << std::endl;
                    }
                }
            }            
        }
        out << circ.root << " 0" << std::endl;
    }
}
