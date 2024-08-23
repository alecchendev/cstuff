#pragma once

#include <stdarg.h>

// Prints a message to the console if DEBUG is defined
void debug(const char* format, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#else
#endif
}
