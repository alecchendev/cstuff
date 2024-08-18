
#include "parse.c"
#include "tokenize.c"
#include "arena.c"

const size_t MAX_MEMORY_SIZE = MAX_INPUT * sizeof(Token) + MAX_INPUT * sizeof(Expression);

// TODO make this + repl more testable..?
bool execute_line(const char *input) {
    Arena *arena = arena_create(MAX_MEMORY_SIZE);
    TokenString tokens = tokenize(input, arena);
    Expression *expr = parse(tokens, arena);
    if (expr == NULL) {
        printf("Invalid expression\n");
    } else if (expr->type == EMPTY) {
        // no op
    } else if (expr->type == QUIT) {
        // is this a case?
        return true;
    } else {
        double result = evaluate(*expr);
        printf("%f\n", result);
    }
    arena_free(arena);
    return false;
}

void repl(FILE *input_fd) {
    char input[MAX_INPUT];
    bool done = false;
    while (!done) {
        printf("%s", prompt);
        if (fgets(input, sizeof(input), input_fd) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = '\0';
        done = execute_line(input);
    }
}

int main(int argc, char **argv) {
    if (argc == 1) {
        repl(stdin);
        return 0;
    } else if (argc == 2) {
        const char *input = argv[1];
        execute_line(input);
    } else {
        printf("Usage: %s [input in quotes]\n", argv[0]);
    }
    return 0;
}
