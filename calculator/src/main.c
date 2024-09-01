
#include <stdio.h>
#include "execute.c"

int main(int argc, char **argv) {
    if (argc == 1) {
        repl(stdin);
    } else if (argc == 2) {
        Arena arena = arena_create();
        Memory memory = memory_new(&arena);
        char output[512] = {0};
        execute_line(argv[1], output, sizeof(output), &memory, &arena);
        if (strnlen(output, sizeof(output)) > 0) printf("%s\n", output);
        arena_free(&arena);
    } else {
        printf("Usage: %s [input in quotes]\n", argv[0]);
    }
    return 0;
}
