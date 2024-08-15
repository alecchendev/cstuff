
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
