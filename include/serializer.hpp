#include <map>
#include <string>
#include <cassert>
#include "logging.hpp"
#include "qcir_keywords.hpp"
#include "circuit.hpp"

namespace preproq::qcir {

    using VarId = unsigned;
    using Literal = int;
       
    #define PREFIX "[QCIR at " << target << ":" << line << ":" << col <<"] "
    #define QCIR_KEYWORD_OUTPUT 5

    class QcirSerializer {
        std::istream& input;        
        std::string target;
        Circuit& circ;

        size_t line = 1;
        size_t col = 0;
        int cur = 0;

        VarId nextId = 1;

        std::string buffer;
        std::string output;
        bool outputPhase = true;

        std::map<std::string, VarId> mapper;

        void next() {
            if(isEof())
                return;
            cur = input.get();
            col++;
            if(cur== '\n') {
                line++;
                col = 0;
            }
        }       

        inline bool check(char c) {
            if(cur == c) {
                next();
                return true;
            }
            else{
                PERR("Expected " << c << " but got " << (char)cur << " (" << cur << ")");
                return false;
            }
        }

        inline void whitespace() {
            while(!isEof() && (cur == ' ' || cur == '\t')) 
                next();
        }

        inline void newline() {
            while(!isEof() && (cur == '\r' || cur == '\n')) 
                next();
        }

        inline bool isNumeric() {
            return cur >= '0' && cur <= '9';
        }

        inline bool isAlphabetical() {
            return (cur >= 'a' && cur <= 'z') || (cur >= 'A' && cur <= 'Z');
        }

        template<typename T>
        inline T readNum(){
            T nr = 0;
            while(isNumeric() && !isEof()){
                nr = nr * 10 + (cur - '0');
                next();
            }
            return nr;
        }

        inline bool inVar() {
            return isNumeric() || isAlphabetical() || (cur == '_');
        }

        inline void readVar() {
            buffer.clear();
            while(inVar()) {
                buffer += cur;
                next();
            }        
        }

        inline int isKeyword() {
            Keyword* ptr = QcirKeywords::isKeyword(buffer.c_str(), buffer.size());
            return ptr != 0 ? ptr->id : 0;
        }

        inline VarId registerVar()  {
            VarId vid = nextId++;
            mapper[buffer] = vid;
            PDBG("Translation " << buffer << " -> " << vid);
            return vid;
        }


    public:
        QcirSerializer(std::istream& input, Circuit& circuit, std::string target = "<internal>") : input(input), target(target), circ(circuit) { 
            next();
        }
        
        bool isEof() {return cur == EOF;}
    
        int formatId(size_t& expected ) {
            expected = 0;
            if(!check('#') || 
               !check('Q') ||
               !check('C') ||
               !check('I') ||
               !check('R') ||
               !check('-'))
               return PREPROQ_ERROR;

            //QCIR-G, expecting values
            bool gVersion = cur == 'G';
            if(gVersion)
                next();
    
            unsigned version = readNum<unsigned>();

            PINF("Detected version " << (gVersion ? "G" : "") << version);
            
            if(gVersion) {
                whitespace();
                expected = readNum<size_t>();
                PINF("Expecting " << expected << " Variables");
            }

            whitespace();
            newline();
            return PREPROQ_OK;
        }     


        int quantifier(QType qtype) {
            whitespace();
            if(!check('(')) return PREPROQ_ERROR;
            do {
                whitespace();
                readVar();
                assert(buffer != "");
                if(mapper.find(buffer) != mapper.end()){
                    PERR("Variable " << buffer << " is already allocated!");
                    return PREPROQ_ERROR;
                }
                VarId vid = registerVar();
                circ.addVar(vid, qtype);

                whitespace();
                if(cur == ',') next();
                else break;
            }   
            while(!isEof());
            whitespace();
            if(!check(')')) return PREPROQ_ERROR;
            whitespace();
            newline();
            return PREPROQ_OK;
        }

        int outputSpec() {
            whitespace();
            if(!check('(')) return PREPROQ_ERROR;

            outputPhase = cur != '-';
            if(!outputPhase)
                next();
            readVar();
            output = buffer;

            whitespace();
            if(!check(')')) return PREPROQ_ERROR;
            whitespace();
            newline();
            return PREPROQ_OK;
        }

        int gate() {
            if(mapper.find(buffer) != mapper.end()) {
                PERR("Specified gate var " << buffer << " is already existing");
                return PREPROQ_ERROR;
            }
            VarId vid = registerVar();

            whitespace();
            if(!check('=')) return PREPROQ_ERROR;            
            whitespace();

            readVar();
            int kid = isKeyword();

            GType gtype = GT_And;
            switch (kid)
            {
            case 6:
                gtype = GT_And;
                break;
            case 7:
                gtype = GT_Or;
                break;
            
            default:
                break;
            }

            circ.addGate(vid, gtype);
            
            if(!check('(')) return PREPROQ_ERROR;
            do {
                whitespace();

                if(cur == ')')
                    break;
                
                bool neg = cur == '-';
                if(neg) next();
                
                readVar();

                assert(buffer != "");
                auto it = mapper.find(buffer);
                if(it == mapper.end()){
                    PERR("Variable " << buffer << " is unknown at this point!");
                    return PREPROQ_ERROR;
                } 
                VarId child = it->second;
                circ.pushBackChild(neg ? -child : child);

                whitespace();
                if(cur == ',') next();
                else break;
            }while(!isEof());
            
            if(!check(')')) return PREPROQ_ERROR;
            whitespace();
            newline();
            return PREPROQ_OK;
            
        }

        int stream() {
            size_t gexpect =0;
            
            bool qblockoccurred = false;

            if(formatId(gexpect) != PREPROQ_OK)
                return PREPROQ_ERROR;

            do {
                readVar();
                int kid = isKeyword();
                if(kid > 0) {
                    QType qt = Free;
                    switch (kid)
                    {
                    case Free: 
                        if(qblockoccurred) {
                            PERR("Free block is only allowed as the very first quantifier block!");
                            return PREPROQ_ERROR;
                        }
                    case QCIR_KW_FORALL:
                    case QCIR_KW_EXISTS: 
                        qt = kid == QCIR_KW_FORALL ? Forall : Exists;
                        qblockoccurred = true;
                        if(quantifier(qt) != PREPROQ_OK) return PREPROQ_ERROR;
                        break;                    
                    case QCIR_KEYWORD_OUTPUT: 
                        if(outputSpec() != PREPROQ_OK) return PREPROQ_ERROR;
                        break;
                    default:
                        ERR("Keyword " << buffer << " received when quantifier expected!");
                        return PREPROQ_ERROR;
                    }
                }
                else {
                    bool isOutput = buffer == output;
                    if(gate() != PREPROQ_OK) return PREPROQ_ERROR;
                    if(isOutput) {
                        PINF("Output gate was parsed, terminating!");
                        circ.root = outputPhase ? mapper[output] : - mapper[output];
                        return PREPROQ_OK;
                    }
                }
            }   
            while(!isEof());
            PERR("Reached end of file without finding output gate!");
            return PREPROQ_ERROR;
        }        
    };
}
#undef PREFIX
