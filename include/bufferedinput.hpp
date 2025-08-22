#pragma once
#include <format>
#include <istream>
#include <string>
#include "logging.hpp"

namespace preproq{
#define BUFFERED_INPUT_BUFSIZE 4096
    class BufferedInput{
        std::istream& fp;
        int eof:1 = 0;
        char buffer[BUFFERED_INPUT_BUFSIZE] = {0};
        size_t bufsize = 0;
        size_t bufptr = 0;
        std::string source;

        void next_buffer(){
            this->fp.read(buffer, BUFFERED_INPUT_BUFSIZE);
            std::size_t read = this->fp.gcount();
            eof = read < BUFFERED_INPUT_BUFSIZE;
            bufsize = read;
            PTR("[BufferedInput] Read " << read << " bytes");
            bufptr = 0;
        }

    public:
        BufferedInput(std::istream& inp, std::string source = "<internal>") : fp(inp), source(source) {
            next_buffer();
            if(cur() == '\n'){
                col = 0;
                line++;
            }
        }
        size_t line = 1;
        size_t col = 1;

        void next(){
            if(!is_eof()){
                if(bufptr >= bufsize-1)
                    next_buffer();
                else 
                    bufptr++;
                if(cur() == '\n'){
                    col = 0;
                    line++;
                }
                else col++;
            }
        }
        inline bool check_and_advance(char ch){
            bool result = equal(ch);
            if(result) next();
            return result;
        }

        inline int is_eof(){ return this->eof && bufptr >= bufsize; }
        inline char cur() const {return this->buffer[bufptr];}

        inline bool equal(char ch){return cur() == ch;}
        inline bool is_whitespace() { return !is_eof() && (equal(' ') || equal('\t')); }
        inline bool is_newline() { return !is_eof() && (equal('\n') || equal('\r')); }
        inline bool is_numeric() {return !is_eof() && cur() >= '0' && cur() <= '9'; }
        inline bool is_integer() {return !is_eof() && (is_numeric() || equal('-')); }
        
        inline void skip_whitespaces(){ while(is_whitespace()) next(); }
        inline void skip_newlines(){ while(is_newline()) next(); }

        template<typename T>
        inline int read_number(){
            T nr = 0;
            while(is_numeric() && !is_eof()){
                nr = nr * 10 + (cur() - '0');
                next();
            }
            return nr;
        }

        template<typename T>
        inline int read_integer(){
            T factor = 1;
            if(equal('-')){
                factor = -1;
                next();
            }
            return read_number<T>() * factor;
        }

        inline std::string location (){
            return std::format("{}:{}:{} ", source, line, col);
        }
    };
}
