
#include "parse.c"
#include "tokenize.c"
#include "arena.c"

int main() {
    char input[MAX_INPUT];
    FILE *input_fd = stdin;
    bool done = false;
    while (!done) {
        printf("%s", prompt);
        if (fgets(input, sizeof(input), input_fd) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = '\0';
        Arena *arena = arena_create(MAX_MEMORY_SIZE);
        TokenString tokens = tokenize(input, arena);

        for (size_t i = 0; i < tokens.length; i++) {
            
        }

        /*Expression *expr = parse(input, arena);*/
        /*if (expr == NULL) {*/
        /*    printf("Invalid expression\n");*/
        /*    continue;*/
        /*}*/
        /*if (expr->type == QUIT) {*/
        /*    break;*/
        /*}*/
        /*double result = evaluate(*expr);*/
        /*printf("%f\n", result);*/
        arena_free(arena);
    }
    return 0;
}
