
#include <stdio.h>
#include "execute.c"

int main(int argc, char **argv) {
    if (argc == 1) {
        repl(stdin);
    } else if (argc == 2) {
        char output[256] = {0};
        execute_line(argv[1], output, sizeof(output));
        if (strnlen(output, sizeof(output)) > 0) printf("%s\n", output);
    } else {
        printf("Usage: %s [input in quotes]\n", argv[0]);
    }
    return 0;
}
