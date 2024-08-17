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
    EMPTY,
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

Expression *parse(TokenString tokens, Arena *arena) {
    if (tokens.length == 0 || (tokens.length == 1 && tokens.tokens[0].type == END)) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        *expr = (Expression) { .type = EMPTY };
        /*printf("empty\n");*/
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == QUITTOKEN) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        *expr = (Expression) { .type = QUIT };
        /*printf("quit\n");*/
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == NUMBER) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        expr->type = CONSTANT;
        expr->expr.constant.value = tokens.tokens[0].data.number;
        /*printf("constant\n");*/
        return expr;
    }
    if (tokens.length == 1) {
        /*printf("Invalid expression token length 1\n");*/
        return NULL;
    }
    if (tokens.length == 2 && tokens.tokens[1].type == END) {
        /*printf("Parsing end token\n");*/
        return parse((TokenString) { .tokens = tokens.tokens, .length = 1 }, arena);
    }

    BinaryOp op = NOOP;
    size_t op_index = 0;
    for (int i = 0; i < tokens.length; i++) {
        if (tokens.tokens[i].type == BINARY_OPERATOR) {
            op = tokens.tokens[i].data.binary_operator;
            op_index = i;
            break;
        }
    }
    if (op != NOOP && (op_index == 0 || op_index == tokens.length - 1)) {
        /*printf("Invalid expression found operator but bad length\n");*/
        return NULL;
    }
    if (op != NOOP) {
        /*printf("found operator %d at %zu\n", op, op_index);*/
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        expr->type = BINARY_OP;
        expr->expr.binary_expr.operator = op;

        TokenString left_tokens = { .tokens = tokens.tokens, .length = op_index };
        Expression *left = parse(left_tokens, arena);
        if (left == NULL || left->type == EMPTY || left->type == QUIT) {
            return left;
        }
        expr->expr.binary_expr.left = left;

        TokenString right_tokens = { .tokens = tokens.tokens + op_index + 1, .length = tokens.length - op_index - 1 };
        Expression *right = parse(right_tokens, arena);
        if (right == NULL || right->type == EMPTY || right->type == QUIT) {
            return right;
        }
        expr->expr.binary_expr.right = right;

        return expr;
    }

    /*printf("Invalid expression no operator found\n");*/
    return NULL;
}

double evaluate(Expression expr) {
    if (expr.type == CONSTANT) {
        return expr.expr.constant.value;
    }
    if (expr.type == BINARY_OP) {
        double left = evaluate(*expr.expr.binary_expr.left);
        double right = evaluate(*expr.expr.binary_expr.right);
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