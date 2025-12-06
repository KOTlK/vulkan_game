#pragma once

#ifdef DEBUG
    #include <stdio.h>

    #define Log(msg) fprintf(stderr, "LOG: %s %s : %i\n", msg, __FILE__, __LINE__);
    #define Err(msg) fprintf(stderr, "ERROR: %s %s : %i\n", msg, __FILE__, __LINE__);

    #define Logf(msg, ...) \
            fprintf(stderr, "LOG: ");\
            fprintf(stderr, msg, ##__VA_ARGS__);\
            fprintf(stderr, " %s : %i\n", __FILE__, __LINE__);\

    #define Errf(msg, ...) \
            fprintf(stderr, "ERROR: ");\
            fprintf(stderr, msg, ##__VA_ARGS__);\
            fprintf(stderr, " %s : %i\n", __FILE__, __LINE__);\

#else
    #define Log(msg) 
    #define Err(msg) 
    #define Logf(msg, ...)
    #define Errf(msg, ...)
#endif