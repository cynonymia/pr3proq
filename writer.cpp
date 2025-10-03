#include "writer.hpp"

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

    static size_t precalculate(Circuit& circ) {
        size_t count = 0;
        for(VarId vid = circ.gateBegin(); vid < circ.gateEnd(); vid++) {
            GType gt = static_cast<GType>(circ.var(vid).gtype);
            if(gt == GT_And) {

            }
        }
        return count;
    }

    void writeQdimacs(std::ostream& out, Circuit& circ) {
        out << "p cnf " << circ.varEnd() << " " << 1;
    }
}
