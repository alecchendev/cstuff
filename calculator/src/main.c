
#include <stdio.h>
#include "execute.c"

int main(int argc, char **argv) {
    if (argc == 1) {
        repl(stdin);
    } else if (argc == 2) {
        execute_line(argv[1]);
    } else {
        printf("Usage: %s [input in quotes]\n", argv[0]);
    }
    return 0;
}
