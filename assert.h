#pragma once


#ifdef DEBUG
#include <cstdio>
#include <cstdlib>
#endif

#ifdef DEBUG
#define Assert(expr, msg) \
    if (!(expr)) {\
        fprintf(stderr, "\nAssert failed. %s:%i. %s\n", __FILE__, __LINE__, msg);\
        exit(0);\
    }\

#define Assertf(expr, msg, ...) \
    if (!(expr)) {\
        fprintf(stderr, "\nAssert failed. %s:%i. ", __FILE__, __LINE__);\
        fprintf(stderr, msg, ##__VA_ARGS__);\
        fprintf(stderr, "\n");\
        exit(0);\
    }\
    
#else

#define Assert(expr, msg)
#define Assertf(expr, msg, ...) 

#endif