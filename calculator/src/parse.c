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
    UnitType unit;
    ExprType type;
    ExprData expr;
};

Expression expr_new_const_unit(double value, UnitType unit) {
    return (Expression) {
        .unit = unit,
        .type = EXPR_CONSTANT,
        .expr = { .constant = { .value = value } },
    };
}

Expression expr_new_const(double value) {
    return expr_new_const_unit(value, UNIT_NONE);
}

// This should only be used for tests, otherwise
// we need to make sure left and right remain valid.
Expression expr_new_bin(ExprType type, Expression left, Expression right) {
    return (Expression) {
        .unit = UNIT_NONE,
        .type = type,
        .expr = { .binary_expr = { .left = &left, .right = &right } },
    };
}

// Precedence - lower number means this should be evaluated first
int precedence(TokenType op) {
    if (op == TOK_MUL || op == TOK_DIV) return 1;
    if (op == TOK_ADD || op == TOK_SUB) return 2;
    return 0; // error case, shouldn't matter
}

Expression *parse(TokenString tokens, Arena *arena) {
    if (tokens.length == 0 || (tokens.length == 1 && tokens.tokens[0].type == TOK_END)) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        *expr = (Expression) { .type = EXPR_EMPTY };
        /*printf("empty\n");*/
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_QUIT) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        *expr = (Expression) { .type = EXPR_QUIT };
        /*printf("quit\n");*/
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_NUM) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        expr->type = EXPR_CONSTANT;
        expr->expr.constant.value = tokens.tokens[0].number;
        expr->unit = UNIT_NONE;
        /*printf("constant\n");*/
        return expr;
    }
    if (tokens.length == 1) {
        /*printf("Invalid expression token length 1\n");*/
        return NULL;
    }
    if (tokens.length > 1 && tokens.tokens[tokens.length - 1].type == TOK_END) {
        /*printf("Parsing end token\n");*/
        return parse((TokenString) { .tokens = tokens.tokens, .length = tokens.length - 1 }, arena);
    }
    if (tokens.length == 2 && tokens.tokens[0].type == TOK_NUM
        && tokens.tokens[1].type == TOK_UNIT) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        expr->type = EXPR_CONSTANT;
        expr->expr.constant.value = tokens.tokens[0].number;
        expr->unit = tokens.tokens[1].unit;
        /*printf("constant with unit\n");*/
        return expr;
    }

    // Anything remaining should be a binary expression.

    // Find last case of highest precedence.
    // When we evaluate, we evaluate the leaves first, so
    // when we're creating the root here, we want the thing
    // that will be evaluated last.
    size_t op_index = 0;
    for (int i = 0; i < tokens.length; i++) {
        int curr_precedence = precedence(tokens.tokens[i].type);
        int best_precedence = precedence(tokens.tokens[op_index].type);
        if (curr_precedence >= best_precedence) {
            op_index = i;
        }
    }
    if (op_index == 0 || op_index == tokens.length - 1) {
        return NULL;
    }
    Expression *expr = arena_alloc(arena, sizeof(Expression));
    TokenType op = tokens.tokens[op_index].type;
    if (op == TOK_ADD) {
        expr->type = EXPR_ADD;
    } else if (op == TOK_SUB) {
        expr->type = EXPR_SUB;
    } else if (op == TOK_MUL) {
        expr->type = EXPR_MUL;
    } else if (op == TOK_DIV) {
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

    if (left->unit == UNIT_NONE) {
        expr->unit = right->unit;
    } else if (right->unit == UNIT_NONE) {
        expr->unit = left->unit;
    } else if (left->unit == right->unit) {
        expr->unit = left->unit;
    } else {
        expr->unit = UNIT_UNKNOWN; // TODO: implement composite units
    }

    return expr;
}

unsigned char display_expr_op(ExprType type) {
    switch (type) {
        case EXPR_ADD:
            return '+';
        case EXPR_SUB:
            return '-';
        case EXPR_MUL:
            return '*';
        case EXPR_DIV:
            return '/';
        default:
            return '?';
    }
}

bool check(Expression expr, Arena *arena) {
    if (expr.type == EXPR_CONSTANT) {
        return true;
    }
    Expression left = *expr.expr.binary_expr.left;
    Expression right = *expr.expr.binary_expr.right;
    bool left_check = check(left, arena);
    bool right_check = check(right, arena);
    if (!left_check || !right_check) {
        return false;
    }
    if ((expr.type == EXPR_ADD || expr.type == EXPR_SUB) &&
        left.unit != UNIT_NONE && right.unit != UNIT_NONE &&
        left.unit != right.unit) {
        printf("Units do not match: %lf %s %c %lf %s\n",
               left.expr.constant.value, unit_strings[left.unit], display_expr_op(expr.type),
               right.expr.constant.value, unit_strings[right.unit]);
        return false;
    }
    return true;
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
