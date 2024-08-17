
#include "parse.c"
#include "tokenize.c"
#include "arena.c"

const size_t MAX_MEMORY_SIZE = MAX_INPUT * sizeof(Token) * MAX_INPUT * sizeof(Expression);

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
        Expression *expr = parse(tokens, arena);
        if (expr == NULL) {
            printf("Invalid expression\n");
        } else if (expr->type == EMPTY) {
            // no op
        } else if (expr->type == QUIT) {
            // is this a case?
            done = true;
        } else {
            double result = evaluate(*expr);
            printf("%f\n", result);
        }
        arena_free(arena);
    }
    return 0;
}
