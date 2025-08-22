#include <map>
#include <string>
#include <cassert>
#include "logging.hpp"
#include "qcir_keywords.hpp"
#include "circuit.hpp"

namespace qser {

    using VarId = unsigned;
    using Literal = int;
       
    #define PREFIX "[" << target << ":" << line << ":" << col <<"]"

    enum QType {
        Free = 2,
        Exists = 3,
        Forall = 4
    };

    #define QCIR_KEYWORD_OUTPUT 5

    class QcirSerializer {
        std::istream& input;        
        std::string target;
        Circuit circ;

        size_t line = 1;
        size_t col = 0;
        int cur = 0;

        VarId nextId = 1;

        std::string buffer;
        std::string output;

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
                PERR("Expected " << c << " but got " << cur);
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
            }        
        }

        inline int isKeyword() {
            Keyword* ptr = QcirKeywords::isKeyword(buffer.c_str(), buffer.size());
            return ptr != 0 ? ptr->id : 0;
        }

        inline void writeBqcir(BqcirElem elem) {
            assert(consumer != nullptr);
            (*consumer)(elem);
        }


        inline VarId registerVar()  {
            VarId vid = nextId++;
            mapper[buffer] = vid;
            PDBG("Translation " << buffer << " -> " << nextId);
        }


    public:
        QcirSerializer(std::istream& input, std::string target, bqcirConsumer* consumer) : input(input), target(target), consumer(consumer) { 
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
               return QSER_ERROR;

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

            writeBqcir(BQCIR_V1);
            whitespace();
            newline();
            return QSER_OK;
        }     


        int quantifier(QType qtype) {
            whitespace();
            if(!check('(')) return QSER_ERROR;
            do {
                whitespace();
                readVar();
                assert(buffer != "");
                if(mapper.find(buffer) != mapper.end()){
                    PERR("Variable " << buffer << " is already allocated!");
                    return QSER_ERROR;
                }
                VarId vid = registerVar();
                writeBqcir(qtype == Forall ? -vid : vid);

                whitespace();
                if(cur == ',') next();
                else break;
            }   
            while(!isEof());
            whitespace();
            if(!check(')')) return QSER_ERROR;
            whitespace();
            newline();
            return QSER_OK;
        }

        int outputSpec() {
            whitespace();
            if(!check('(')) return QSER_ERROR;

            readVar();
            output = buffer;

            writeBqcir(0);  //end of Prefix

            whitespace();
            if(!check(')')) return QSER_ERROR;
            whitespace();
            newline();
            return QSER_OK;
        }

        int gate() {
            if(mapper.find(buffer) != mapper.end()) {
                PERR("Specified gate var " << buffer << " is already existing");
                return QSER_ERROR;
            }
            VarId vid = registerVar();

            whitespace();
            if(!check('=')) return QSER_ERROR;            
            whitespace();

            readVar();
            int kid = isKeyword();

            BqcirGate gtype = And;
            switch (kid)
            {
            case And: case Or:
                gtype = qcir_to_bqcir[kid];
                break;
            
            default:
                break;
            }

            if(!check('(')) return QSER_ERROR;
            do {
                whitespace();
                readVar();
                assert(buffer != "");
                auto it = mapper.find(buffer);
                if(it == mapper.end()){
                    PERR("Variable " << buffer << " is unknown at this point!");
                    return QSER_ERROR;
                } 
                VarId child = it->second;


                whitespace();
                if(cur == ',') next();
                else break;
            }while(!isEof());

        }

        int stream() {
            size_t gexpect =0;
            
            bool qblockoccurred = false;

            if(formatId(gexpect) != QSER_OK)
                return QSER_ERROR;

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
                            return QSER_ERROR;
                        }
                    case Forall: 
                    case Exists: 
                        qt = static_cast<QType>(kid);
                        qblockoccurred = true;
                        if(quantifier(qt) != QSER_OK) return QSER_ERROR;
                        break;                    
                    case QCIR_KEYWORD_OUTPUT: 
                        if(outputSpec() != QSER_OK) return QSER_ERROR;
                        break;
                    default:
                        ERR("Keyword " << buffer << " received when quantifier expected!");
                        return QSER_ERROR;
                    }
                }
                else {
                    bool isOutput = buffer == output;
                    if(gate() != QSER_OK) return QSER_ERROR;
                    if(isOutput) {
                        PINF("Output gate was parsed, terminating!");
                        return QSER_OK;
                    }
                }
            }   
            while(!isEof());         

        }        
    };
}
