
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Expression Expression;
typedef struct Constant Constant;
typedef struct BinaryExpr BinaryExpr;

struct Constant {
    double value;
};

typedef enum {
    NOOP,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
} BinaryOp;

struct BinaryExpr {
    BinaryOp operator;
    Expression *left;
    Expression *right;
};

typedef enum {
    CONSTANT,
    BINARY_OP,
    QUIT,
} ExprType;

typedef union {
    Constant constant;
    BinaryExpr binary_expr;
} ExprData;

struct Expression {
    ExprType type;
    ExprData expr;
};

typedef struct Arena Arena;

struct Arena {
    size_t size;
    size_t used;
    char memory[];
};

Arena *arena_create(size_t size) {
    Arena *arena = malloc(sizeof(Arena) + size);
    arena->size = size;
    arena->used = 0;
    // arena->memory already points to the start of the memory block
    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    if (arena->used + size > arena->size) {
        return NULL; // TODO
    }
    void *ptr = arena->memory + arena->used;
    arena->used += size;
    return ptr;
}

void arena_free(Arena *arena) {
    free(arena);
}

const unsigned int MAX_INPUT = 256;
const char *prompt = ">>> ";

enum TokenType {
    NUMBER,
    BINARY_OPERATOR,
    WHITESPACE,
    QUITTOKEN, // TODO: how to name QUIT without conflicts
    END,
    INVALID,
};

typedef union TokenData TokenData;
union TokenData {
    double number;
    BinaryOp binary_operator;
};

typedef struct Token Token;
struct Token {
    enum TokenType type;
    union TokenData data;
};

// TODO: turn this into something simpler at comptime
bool char_in_set(char c, const unsigned char *set) {
    bool char_set[256] = {0};
    for (size_t i = 0; set[i] != '\0'; i++) {
        char_set[set[i]] = true;
    }
    return char_set[c];
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

double char_to_digit(char c) {
    return c - '0';
}

Token next_token(char *input, size_t *pos, size_t length) {
    const Token invalid_token = {INVALID};
    const Token end_token = {END};
    const unsigned char end[] = {'\0', '\n'};
    if (*pos >= length || char_in_set(input[*pos], end)) {
        if (*pos != length) return invalid_token;
        return end_token;
    }

    if (input[*pos] == 'q' || input[*pos] == 'e') {
        // TODO: handle no spaces after?
        if (strncmp(input + *pos, "quit", 4) == 0 || strncmp(input + *pos, "exit", 4) == 0) {
            Token quit_token = {QUITTOKEN};
            *pos += 4;
            return quit_token;
        }
    }

    const Token whitespace_token = {WHITESPACE};
    const unsigned char whitespace[] = {' ', '\t'};
    if (char_in_set(input[*pos], whitespace)) {
        while (char_in_set(input[*pos], whitespace)) {
            (*pos)++;
        }
        return whitespace_token;
    }

    const unsigned char operators[] = {'+', '-', '*', '/'};
    if (char_in_set(input[*pos], operators)) {
        TokenData data;
        switch (input[*pos]) {
            case '+':
                data.binary_operator = ADD;
                break;
            case '-':
                data.binary_operator = SUBTRACT;
                break;
            case '*':
                data.binary_operator = MULTIPLY;
                break;
            case '/':
                data.binary_operator = DIVIDE;
                break;
        }
        Token bin_token = {BINARY_OPERATOR, data};
        (*pos)++;
        return bin_token;
    }

    if (is_digit(input[*pos])) {
        double number = 0;
        while (is_digit(input[*pos])) {
            number = number * 10 + char_to_digit(input[*pos]);
            (*pos)++;
        }
        if (input[*pos] == '.') {
            (*pos)++;
            double decimal = 0.1;
            while (input[*pos] >= '0' && input[*pos] <= '9') {
                number += (input[*pos] - '0') * decimal;
                decimal /= 10;
                (*pos)++;
            }
        }
        TokenData data = { .number = number };
        Token num_token = {NUMBER, data};
        return num_token;
    }

    return invalid_token;
}

Expression *parse(char *input, Arena *arena) {
    Expression *expr = arena_alloc(arena, sizeof(Expression));
    if (strnlen(input, MAX_INPUT) == 4 && (strncmp(input, "quit", 4) == 0 || strncmp(input, "exit", 4) == 0)) {
        expr->type = QUIT;
        return expr;
    }
    // Parse binary expression
    double left_value, right_value;
    char op;
    BinaryOp operator = NOOP;
    if (sscanf(input, "%lf %c %lf", &left_value, &op, &right_value) == 3) {
        switch (op) {
            case '+':
                operator = ADD;
                break;
            case '-':
                operator = SUBTRACT;
                break;
            case '*':
                operator = MULTIPLY;
                break;
            case '/':
                operator = DIVIDE;
                break;
        }
    }
    if (operator != NOOP) {
        BinaryExpr binary_expr;
        binary_expr.operator = operator;

        binary_expr.left = arena_alloc(arena, sizeof(Expression));
        binary_expr.left->type = CONSTANT;
        binary_expr.left->expr.constant.value = left_value;

        binary_expr.right = arena_alloc(arena, sizeof(Expression));
        binary_expr.right->type = CONSTANT;
        binary_expr.right->expr.constant.value = right_value;

        expr->type = BINARY_OP;
        expr->expr.binary_expr = binary_expr;
        return expr;
    }

    // Parse single constant
    double value;
    if (sscanf(input, "%lf", &value) == 1) {
        Constant constant = { .value = value };
        expr->type = CONSTANT;
        expr->expr.constant = constant;
        return expr;
    }

    return NULL;
}

double evaluate(Expression expr) {
    if (expr.type == CONSTANT) {
        return expr.expr.constant.value;
    }
    if (expr.type == BINARY_OP) {
        double left = evaluate(*expr.expr.binary_expr.left);
        double right = evaluate(*expr.expr.binary_expr.right);
        printf("%f %f\n", left, right);
        switch (expr.expr.binary_expr.operator) {
            case ADD:
                return left + right;
            case SUBTRACT:
                return left - right;
            case MULTIPLY:
                return left * right;
            case DIVIDE:
                return left / right;
            case NOOP:
                exit(1); // TODO
        }
    }
    exit(1); // TODO
}

int main() {
    char input[MAX_INPUT];
    FILE *input_fd = stdin;
    while (1) {
        printf("%s", prompt);
        if (fgets(input, sizeof(input), input_fd) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = '\0';
        Arena *arena = arena_create(1024);
        Expression *expr = parse(input, arena);
        if (expr == NULL) {
            printf("Invalid expression\n");
            continue;
        }
        if (expr->type == QUIT) {
            break;
        }
        double result = evaluate(*expr);
        printf("%f\n", result);
        arena_free(arena);
    }
    return 0;
}
