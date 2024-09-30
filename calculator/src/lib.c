#include <emscripten.h>
#include "execute.c"

// This is ugly/hacky. TODO: get rid of this
// and clean up general wasm interop.
bool initialized = false;
Memory *memory = NULL;
Arena *repl_arena = NULL;

EMSCRIPTEN_KEEPALIVE
bool exported_execute_line(const char *input, char *output, size_t output_len) {
    if (!initialized) {
        repl_arena = malloc(sizeof(Arena));
        *repl_arena = arena_create();
        memory = malloc(sizeof(Memory));
        *memory = memory_new(repl_arena);
        initialized = true;
    }
    return execute_line(input, output, output_len, memory, repl_arena);
}
