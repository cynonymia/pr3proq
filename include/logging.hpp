#pragma once

#include <iostream>
#include <vector>
#include "globals.hpp"

#define VERBOSE_SILENT 0
#define VERBOSE_ERROR 1
#define VERBOSE_WARNING 5
#define VERBOSE_INFO 10
#define VERBOSE_DEBUG 50
#define VERBOSE_POINTER_LEVEL 100

extern int global_verbose;

#ifdef COLORIZED_OUTPUT

#define COLOR_RESET   "\033[0m"
#define GREY    "\033[90m"      /* Grey */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define WHITE   "\033[37m"      /* White */

#else

#define COLOR_RESET ""
#define RED         ""
#define GREEN       ""
#define YELLOW      ""
#define BLUE        ""
#define WHITE       ""

#endif


#define LOG_ERROR (global_verbose >= VERBOSE_ERROR)
#define LOG_WARNING (global_verbose >= VERBOSE_WARNING)
#define LOG_INFO (global_verbose >= VERBOSE_INFO)

#define LOG_DBG (global_verbose >= VERBOSE_DEBUG)
#define LOG_PTR (global_verbose >= VERBOSE_POINTER_LEVEL)

#define ERR(x) if(LOG_ERROR) do {std::cerr << RED <<  "[PreProQ] [ERR] " << x << COLOR_RESET << std::endl; } while(0)
#define WRN(x) if(LOG_WARNING) do {std::cerr << YELLOW <<  "[PreProQ] [WRN] " << x << COLOR_RESET << std::endl; } while(0)
#define INF(x) if(LOG_INFO)  do {std::cerr << "[PreProQ] [INF] " << x << std::endl; } while(0)
#ifdef NDEBUG
    #define DBG(x)
    #define PTR(x)
#else
    #define DBG(x) if(LOG_DBG)  do {std::cerr << GREEN <<  "[PreProQ] [DBG] " << x << COLOR_RESET << std::endl ; } while(0)
    #define PTR(x) if(LOG_PTR)  do {std::cerr << GREY <<  "[PreProQ] [PTR] " << x << COLOR_RESET << std::endl; } while(0)
#endif

#ifdef TRACE_INTERFACE
    #define INTR(x) do {std::cerr << x << std::endl;} while(0)
#else
    #define INTR(x)
#endif

//Prefixed Versions (PREFIX has to be defined)
#define PERR(x) ERR(PREFIX << x)
#define PINF(x) INF(PREFIX << x)
#ifdef NDEBUG
    #define PDBG(x)
    #define PPTR(x)
#else
    #define PDBG(x) DBG(PREFIX << x)
    #define PPTR(x) PTR(PREFIX << x)
#endif


#define WRITE(x) std::cout << x
#define WRITELN(x) std::cout << x << std::endl
#define WRITE_CHAR(c) c << " (" << ((int)c) << ")"

std::ostream& operator<< (std::ostream& out, const std::vector<int>& v);
