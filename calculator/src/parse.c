#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tokenize.c"
#include "arena.c"

typedef struct Expression Expression;
typedef struct Constant Constant;
typedef struct BinaryExpr BinaryExpr;

struct Constant {
    double value;
};

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

Expression *parse(char *input, Arena *arena) {
    Expression *expr = arena_alloc(arena, sizeof(Expression));
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
                operator = SUB;
                break;
            case '*':
                operator = MUL;
                break;
            case '/':
                operator = DIV;
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
            case SUB:
                return left - right;
            case MUL:
                return left * right;
            case DIV:
                return left / right;
            case NOOP:
                exit(1); // TODO
        }
    }
    exit(1); // TODO
}
