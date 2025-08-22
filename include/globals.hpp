#pragma once
//if specified, the console will print different colors for each logging level
#define COLORIZED_OUTPUT

#define PREPROQ_ERROR -1
#define PREPROQ_OK 0
#define PREPROQ_SAT 10
#define PREPROQ_UNSAT 20

//Code to be executed in case of an critical error
#define PANIC exit(PREPROQ_ERROR)

#define ERROR_IF(cond, msg) if(cond) { ERR(msg); return PREPROQ_ERROR;}
#define PERROR_IF(cond, msg) if(cond) { PERR(msg); return PREPROQ_ERROR;}
