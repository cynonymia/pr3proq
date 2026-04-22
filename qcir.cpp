#include <compare>
#include <boost/unordered/unordered_flat_map.hpp>
#include "qcir_keywords.hpp"
#include "circuit.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "qcir.hpp"

namespace preproq::qcir {

#define PREFIX "[QCIR at " << target << ":" << line << ":" << col << "] "    

#define EXPECT(ex) do { PERROR_IF(cur != ex, "Expected " << ex << " but instead got " << cur) else next(); } while(0)

    class FastString {
        char* buffer;
        size_t ptr = 0;
        size_t capacity = 0;

    public:
        FastString(size_t initial_capacity) : capacity(initial_capacity) {
            buffer = static_cast<char*>(malloc(initial_capacity * sizeof(char)));
        }
        ~FastString() {
            free(buffer);
        }

        void push_back(char c) {
            if(ptr >= capacity) {
                capacity *= 2;
                buffer = static_cast<char*>(realloc(buffer, capacity * sizeof(char)));
            }
            buffer[ptr++] = c;
        }

        void finish() {
            buffer[ptr] = 0;
        }

        inline size_t size() {
            return ptr;
        }

        void clear() {
            ptr = 0;
        }

        inline char* internal() {
            return buffer;
        }

    };

    class QcirParser {
        FILE* inp;
        Circuit& circ;
        std::string target;
        size_t line = 1;
        size_t col = 0;
        int cur = 0;

        VarId nextVar = 1;
        boost::unordered_flat_map<std::string, VarId> translator;
        
        std::string outputName;
        bool outputNeg = false;

        FastString buffer;


        VarId registerVar() {            
            VarId nvid = nextVar++;
            translator[buffer.internal()] = nvid;
            PPTR("Register " << buffer.internal() << " -> " << nvid);
            return nvid;
        }
        
        bool isEof() {return cur == EOF;}
        
        inline void next_unchecked() {
            cur = fgetc_unlocked(inp);
            if(cur == '\n') {
                col = 0; 
                line++;
            }
            else
                col++;
        }

        inline void next() {
            if(isEof()) return;
            next_unchecked();
        }
       
        inline int isKeyword() {
            Keyword* k = QcirKeywords::isKeyword(buffer.internal(), buffer.size());
            return !k ? 0 : k->id;
        }

        inline bool isFuzzyAlphaNum() {
            return (cur >= '0') && (cur <= 'z') && (cur != '=');
        }

        void skipComments() {
            while(cur == '#') {
                PPTR("Skipping comment");
                while(!isEof() && (cur != '\n'))
                    next_unchecked();
                PPTR("Comment skipped");
                next_unchecked();
            }
        }

        inline void skipwhitespace() {
            while(cur == ' ')
                next_unchecked();            
        }

        inline void skipNewline() {
            while(cur == '\r' || cur == '\n')
                next_unchecked();
        }

        void readVar() {
            buffer.clear();
            while(!isEof() && isFuzzyAlphaNum()) { 
                buffer.push_back(cur);
                next_unchecked();
            }
            buffer.finish();
        }

        int parseOutput() {
            if(cur != '(') {
                PERR("Expected '(' after output definition, but got " << cur);
                return PREPROQ_ERROR;
            }      
            next_unchecked();

            outputName.clear();
            outputNeg = cur == '-';
            if(outputNeg)
                next_unchecked();
            
            while(!isEof() && isFuzzyAlphaNum()) { 
                outputName.push_back(cur);
                next_unchecked();
            }
            PDBG("Read output as " << outputName);

            if(cur != ')') {
                PERR("Expected ')' after output definition, but got " << cur);
                return PREPROQ_ERROR;
            }
            next_unchecked();
            skipwhitespace();
            skipNewline();
            return PREPROQ_OK;
        }
        
        int parseQBlocks() {
            PPTR("Start reading QBlocks");
            while(!isEof()) {
                readVar();
                PPTR("Read keyword: " << buffer.internal());
                QType qtype = Free;
                switch (isKeyword())
                    {
                    case QCIR_KW_FORALL: qtype = Forall; break;
                    case QCIR_KW_EXISTS: qtype = Exists; break;
                    case QCIR_KW_FREE:   qtype = Free;   break;        
                    case QCIR_KW_OUTPUT: return parseOutput();
                    default:
                        PERR("Violation of cleansed form! Unexpected word encountered: "
                             << buffer.internal() << buffer.size()  << ", expected 'forall' or 'exists'");
                        return PREPROQ_ERROR;
                    }
                skipwhitespace();
                PPTR("Start Quantifier block of type " << (int)qtype);
                if(cur != '(') {
                    PERR("Expected '(' after quantifier definition, but got " << WRITE_CHAR(cur));
                    return PREPROQ_ERROR;
                }            
                do {
                    next_unchecked();
                    skipwhitespace();

                    readVar();
                    VarId vid = registerVar();
                    circ.addVar(vid, qtype);
                    skipwhitespace();
                }
                while(cur == ',');
                if(cur != ')') {
                    PERR("Expected ')' after quantifier var definition, but got " << WRITE_CHAR(cur));
                    return PREPROQ_ERROR;
                }
                next_unchecked();
                skipwhitespace();
                skipNewline();
            }
            return PREPROQ_OK;
        }

        int parseGates() {
            while(!isEof()) {
                readVar();
                VarId vid = registerVar();
                skipwhitespace();
                if(cur != '=')  {
                    PERR("Expected '=' after gate variable definition!");
                    return PREPROQ_ERROR;
                }
                next_unchecked();

                skipwhitespace();
                readVar();
                GType gtype = GT_Or;
                switch (isKeyword())
                    {
                    case QCIR_KW_OR:  gtype = GT_Or;  break;
                    case QCIR_KW_AND: gtype = GT_And; break;
                
                    case QCIR_KW_ITE:    case QCIR_KW_XOR: 
                    case QCIR_KW_EXISTS: case QCIR_KW_FORALL:
                        PERR("Currently, only AND and OR gates are allowed (cleansed QCIR)");
                        return PREPROQ_ERROR;

                    default:
                        PERR("Expected gate identifier but got " << buffer.internal());
                        return PREPROQ_ERROR;
                    }

                PPTR("Defining gate " << vid << " type " << (int)gtype);

                circ.addGate(vid, gtype);

                skipwhitespace();
                
                if(cur != '(') {
                    PERR("Expected '(' after gate type identifier, but got " << cur);
                    return PREPROQ_ERROR;
                }

                next_unchecked();
                skipwhitespace();
                
                while(cur != ')') {                    
                    bool neg = cur == '-';
                    if(neg)
                        next_unchecked();
                    readVar();
                    auto it = translator.find(buffer.internal());
                    if(it == translator.end()) {
                        PERR("Referencing variable " << buffer.internal() << " which is unknown at this point!");
                        return PREPROQ_ERROR;
                    }
                    circ.pushBackChild(neg ? -((Literal)it->second) : (Literal)it->second);
                    skipwhitespace();                    
                    if(cur == ',') {
                        next_unchecked();
                        skipwhitespace();
                    }
                    else {
                        break;
                    }
                }
                if(cur != ')') {
                    PERR("Expected ')' after gate children definition, but got " << cur);
                    return PREPROQ_ERROR;
                }  
                next_unchecked();
                skipwhitespace();
                skipNewline();                
            }
            return PREPROQ_OK;
        }

        
    public:

        QcirParser(FILE* inp, Circuit& circ, std::string target) : inp(inp), circ(circ), target(target), buffer(32) {
            next();
        }
        
        int parse() {
            skipComments(); 

            if(parseQBlocks() == PREPROQ_ERROR)
                return PREPROQ_ERROR;

            if(parseGates() == PREPROQ_ERROR)
                return PREPROQ_ERROR;

            assert(isEof());            

            auto it = translator.find(outputName);
            if(it == translator.end()) {
                PERR("Unmapped output symbol '" << outputName << "'");
                return PREPROQ_ERROR;
            }

            circ.root = outputNeg ? -it->second : it->second;            
            PDBG("Mapped back output to " << circ.root);
            return PREPROQ_OK;
        }
        
    };

    int parse(FILE* input, Circuit& circ, std::string target) {
        QcirParser p(input, circ, target);
        return p.parse();        
    }

#undef PREFIX

}
