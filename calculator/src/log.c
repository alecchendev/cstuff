#pragma once

#include <stdarg.h>

// Prints a message to the console if DEBUG is defined
/*void debug(const char* format, ...) {*/
/*#ifdef DEBUG*/
/*    printf("%s:%d %s ", __FILE__, __LINE__, __func__);*/
/*    va_list args;*/
/*    va_start(args, format);*/
/*    vprintf(format, args);*/
/*    va_end(args);*/
/*#else*/
/*#endif*/
/*}*/

// Macro version so we can print metadata
#ifdef DEBUG
#define debug(format, ...) do { \
    printf("%s:%d %s ", __FILE__, __LINE__, __func__); \
    printf(format, ##__VA_ARGS__); \
} while(0)
#else
#define debug(format, ...) ((void)0)
#endif
