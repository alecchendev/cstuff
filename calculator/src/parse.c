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
    Expression *left;
    Expression *right;
};

typedef enum {
    EXPR_CONSTANT,
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_EMPTY,
    EXPR_QUIT,
} ExprType;

typedef union {
    Constant constant;
    BinaryExpr binary_expr;
} ExprData;

struct Expression {
    ExprType type;
    ExprData expr;
};

// Precedence - lower number means this should be evaluated first
int precedence(BinaryOp op) {
    if (op == MUL || op == DIV) return 1;
    if (op == ADD || op == SUB) return 2;
    return 0; // error case, shouldn't matter
}

Expression *parse(TokenString tokens, Arena *arena) {
    if (tokens.length == 0 || (tokens.length == 1 && tokens.tokens[0].type == END)) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        *expr = (Expression) { .type = EXPR_EMPTY };
        /*printf("empty\n");*/
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == QUITTOKEN) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        *expr = (Expression) { .type = EXPR_QUIT };
        /*printf("quit\n");*/
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == NUMBER) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        expr->type = EXPR_CONSTANT;
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

    // Find last case of highest precedence.
    // When we evaluate, we evaluate the leaves first, so
    // when we're creating the root here, we want the thing
    // that will be evaluated last.
    BinaryOp op = NOOP;
    size_t op_index = 0;
    for (int i = 0; i < tokens.length; i++) {
        int curr_precedence = precedence(tokens.tokens[i].data.binary_operator);
        if (tokens.tokens[i].type == BINARY_OPERATOR && curr_precedence >= precedence(op)) {
            op = tokens.tokens[i].data.binary_operator;
            op_index = i;
        }
    }
    if (op != NOOP && (op_index == 0 || op_index == tokens.length - 1)) {
        /*printf("Invalid expression found operator but bad length\n");*/
        return NULL;
    }
    if (op != NOOP) {
        /*printf("found operator %d at %zu\n", op, op_index);*/
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        if (op == ADD) {
            expr->type = EXPR_ADD;
        } else if (op == SUB) {
            expr->type = EXPR_SUB;
        } else if (op == MUL) {
            expr->type = EXPR_MUL;
        } else if (op == DIV) {
            expr->type = EXPR_DIV;
        }

        TokenString left_tokens = { .tokens = tokens.tokens, .length = op_index };
        Expression *left = parse(left_tokens, arena);
        if (left == NULL || left->type == EXPR_EMPTY || left->type == EXPR_QUIT) {
            return left;
        }
        expr->expr.binary_expr.left = left;

        TokenString right_tokens = { .tokens = tokens.tokens + op_index + 1, .length = tokens.length - op_index - 1 };
        Expression *right = parse(right_tokens, arena);
        if (right == NULL || right->type == EXPR_EMPTY || right->type == EXPR_QUIT) {
            return right;
        }
        expr->expr.binary_expr.right = right;

        return expr;
    }

    /*printf("Invalid expression no operator found\n");*/
    return NULL;
}

double evaluate(Expression expr) {
    if (expr.type == EXPR_CONSTANT) {
        return expr.expr.constant.value;
    }
    double left = evaluate(*expr.expr.binary_expr.left);
    double right = evaluate(*expr.expr.binary_expr.right);
    switch (expr.type) {
        case EXPR_ADD:
            return left + right;
        case EXPR_SUB:
            return left - right;
        case EXPR_MUL:
            return left * right;
        case EXPR_DIV:
            return left / right;
        default:
            exit(1); // TODO
    }
}
