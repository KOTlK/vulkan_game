#pragma once


#ifdef DEBUG
#include <cstdio>
#include <cstdlib>

#define Assert(expr, msg) \
    if (!(expr)) {\
        printf("\nAssert failed in:%s, line:%i. %s\n", __FILE__, __LINE__, msg);\
        exit(0);\
    }\

#else

#define Assert(expr, msg)

#endif