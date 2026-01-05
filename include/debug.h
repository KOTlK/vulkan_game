#pragma once

// #define DEBUG_USE_FILE_NAMES before including this header to print
// file name and line number of place where the calling occurs.

#ifdef DEBUG
    #include <stdio.h>

    #ifdef DEBUG_USE_FILE_NAMES
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
        #define Log(msg) fprintf(stderr, "LOG: %s\n", msg);
        #define Err(msg) fprintf(stderr, "ERROR: %s\n", msg);

        #define Logf(msg, ...) \
                fprintf(stderr, "LOG: ");\
                fprintf(stderr, msg, ##__VA_ARGS__);\
                fprintf(stderr, "\n");\

        #define Errf(msg, ...) \
                fprintf(stderr, "ERROR: ");\
                fprintf(stderr, msg, ##__VA_ARGS__);\
                fprintf(stderr, "\n");\

    #endif

#else
    #define Log(msg) 
    #define Err(msg) 
    #define Logf(msg, ...)
    #define Errf(msg, ...)
#endif