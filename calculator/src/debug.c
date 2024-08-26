#pragma once

#include <stdio.h>
#include <stdlib.h>

// Macro so we can print metadata
#ifdef DEBUG
#define debug(format, ...) do { \
    printf("%s:%d %s ", __FILE__, __LINE__, __func__); \
    printf(format, ##__VA_ARGS__); \
} while(0)
#else
#define debug(format, ...) ((void)0)
#endif

#define assert(condition) \
    ((condition) ? (void)0 : \
    __assert_fail(#condition, __FUNCTION__, __FILE__, __LINE__))

void __assert_fail(const char *condition, const char *function, const char *file, 
                   unsigned int line) {
    fprintf(stderr, "Assertion failed: %s, function %s, file %s, line %u.\n",
            condition, function, file, line);
    abort();
}

// TODO: do some sort of generic display to show
// the expected and actual values
#define assert_eq(a, b) assert((a) == (b))

