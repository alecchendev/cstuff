#include <emscripten.h>
#include "execute.c"

EMSCRIPTEN_KEEPALIVE
bool exported_execute_line(const char *input, char *output, size_t output_len) {
    return execute_line(input, output, output_len);
}
