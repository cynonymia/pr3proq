#include "logging.hpp"

int global_verbose = VERBOSE_ERROR;

std::ostream& operator<< (std::ostream& out, const std::vector<int>& c)
{
    out << '[';
    bool first = true;
    for(auto i : c){
        if(first) first = false;
        else out << ",";
        out << i;
    }    
    out << "]";
    return out;
}
