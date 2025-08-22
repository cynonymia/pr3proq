#include <compare>
#include "circuit.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "qcir.hpp"
#include "serializer.hpp"

namespace preproq::qcir {

#define PREFIX "[QcirParse:" << line << ":" << col << "] "    

#define EXPECT(ex) do { PERROR_IF(cur != ex, "Expected " << ex << " but instead got " << cur) else next(); } while(0)
    
    class CleansedParser {
        std::istream& inp;
        Circuit& circ;
        size_t line = 1;
        size_t col = 0;
        int cur = 0;

        std::string buffer;
        
        bool isEof() {return cur == EOF;}
        
        void next() {
            if(isEof())
                return;
            do {
                cur = inp.get();
                col++;
                if(cur== '\n') {
                    line++;
                    col = 0;
                }
            }
            while(isWhiteSpace());
        }

        inline bool isNumber() {
            return cur >= '0' && cur <= '9';
        }

        unsigned readNumber() {
            unsigned n = 0;
            while(!isEof() && isNumber()) {
                n = n * 10 + cur - '0';
                next();
            }
            return n;
        }        

        inline bool isInteger() {
            return isNumber() || cur == '-';
        }

        int readInteger() {
            bool neg = cur == '-';
            if(neg) next();
            int n = readNumber();
            return neg ? -n : n;
        }

        inline bool isText() {
            return (cur >= 'A' && cur <= 'Z')
                || (cur >= 'a' && cur <= 'z');
        }

        void readText() {
            buffer.clear();
            while(!isEof() && isText()) {
                buffer += static_cast<char>(cur);
                next();
            }            
        }

        inline int isKeyword() {
            Keyword* k = QcirKeywords::isKeyword(buffer.c_str(), buffer.size());
            return !k ? 0 : k->id;
        }

        inline bool isWhiteSpace() {
            return cur == ' ' || cur == '\t';
        }

        void skipNewlines() {
            while(!isEof() && (cur == '\r' || cur == '\n'))
                next();
        }

        int parseComments() {
            while(1){                
                if(cur == '#') {
                    PPTR("Skipping comment");
                    while(!isEof() && cur != '\n')
                        next();
                    skipNewlines();
                }
                else
                    break;
            }
            return !isEof() ? PREPROQ_OK : PREPROQ_ERROR;
        }

        int parseQVars(QType qtype) {
            EXPECT('(');
            while(!isEof()) {
                int n = readNumber();
                circ.addVar(VAR(n), qtype);
                if(cur == ',')
                    next();
                else
                    break;
            }
            EXPECT(')');
            skipNewlines();
            return PREPROQ_OK;
        }

        int parseOutput() {
            EXPECT('(');
            ERROR_IF(!isInteger(), "Expected numerical value for output");
            circ.root = readInteger();
            EXPECT(')');
            skipNewlines();
            return PREPROQ_OK;
        }

        int parseQBlocks() { 
            while(!isEof()) {
                if(!isText())
                    return PREPROQ_OK;
                readText();
                int keywordId = isKeyword();
                QType qtype = Free;
                if(keywordId == QCIR_KW_FORALL) 
                    qtype = Forall;
                else if(keywordId == QCIR_KW_EXISTS)
                    qtype = Exists;
                else if(keywordId == QCIR_KW_OUTPUT) {
                    return parseOutput();
                }                
                else {
                    PERR("Violation of cleansed form: Unexpected "
                         << (keywordId == 0 ? "" : "key") << "word encountered: "
                         << buffer << ", expected 'forall' or 'exists'");
                    return PREPROQ_ERROR;
                }
                if(parseQVars(qtype) != PREPROQ_OK)
                    return PREPROQ_ERROR;                
            }
            return PREPROQ_OK;
        }

        int parseGateChildren(VarId vid) {
            EXPECT('(');
            while(!isEof()) {
                int n = readInteger();
                circ.pushBackChild(n);
                if(cur == ',')
                    next();
                else
                    break;
            }
            EXPECT(')');
            skipNewlines();
            return PREPROQ_OK;
        }
        
        int parseGates() {
            while(!isEof()) {
                if(!isNumber())
                    return PREPROQ_OK;
                VarId vid = readNumber();
                EXPECT('=');
                PERROR_IF(!isText(), "Expecting gate modifier at this point");
                readText();
                int keywordId = isKeyword();
                GType gtype = GT_Or;
                if(keywordId == QCIR_KW_OR) {
                    //Already set
                }
                else if(keywordId == QCIR_KW_AND) {
                    gtype = GT_And;
                }
                else if(keywordId == QCIR_KW_ITE || keywordId == QCIR_KW_XOR) {
                    PERR("Violation of cleansed form: ITE and XOR gates are not supported!");
                    return PREPROQ_ERROR;
                }
                else {
                    PERR("Expected AND or OR, instead got " << buffer);
                    return PREPROQ_ERROR;
                }

                circ.addGate(vid, gtype);
                
                if(parseGateChildren(vid) == PREPROQ_ERROR)
                    return PREPROQ_ERROR;                
            }
            return PREPROQ_OK;
        }

        
    public:

        CleansedParser(std::istream& inp, Circuit& circ) : inp(inp), circ(circ) {
            next();
        }
        
        int parse() {
            PERROR_IF(parseComments() == PREPROQ_ERROR,
                  "Encountered EOF during traversing comments");

            if(parseQBlocks() == PREPROQ_ERROR)
                return PREPROQ_ERROR;

            if(parseGates() == PREPROQ_ERROR)
                return PREPROQ_ERROR;

            assert(isEof());            
            
            return PREPROQ_OK;
        }
        
    };

  
    int parse_cleansed(std::istream& input, Circuit& circ) {
        CleansedParser p(input, circ);
        return p.parse();        
    }

#undef PREFIX

    int parse(std::istream& input, Circuit& circ, std::string target) {
        QcirSerializer qser(input, circ, target);
        return qser.stream();
    }
}
