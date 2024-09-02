#pragma once

#include <stdbool.h>
#include "arena.c"
#include "tokenize.c"
#include "unit.c"

typedef struct Expression Expression;
typedef struct Constant Constant;
typedef struct BinaryExpr BinaryExpr;
typedef struct UnaryExpr UnaryExpr;
typedef struct Convert Convert;

struct Constant {
    double value;
};

struct UnaryExpr {
    Expression *right;
};

struct BinaryExpr {
    Expression *left;
    Expression *right;
};

typedef enum {
    EXPR_CONSTANT,
    EXPR_UNIT,
    EXPR_NEG,
    EXPR_CONST_UNIT,
    EXPR_COMP_UNIT,
    EXPR_DIV_UNIT,
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_CONVERT,
    EXPR_POW,
    EXPR_VAR,
    EXPR_SET_VAR,
    EXPR_EMPTY,
    EXPR_QUIT,
    EXPR_HELP,
    EXPR_INVALID,
} ExprType;

typedef union {
    double constant;
    unsigned char *var_name;
    Unit unit;
    UnaryExpr unary_expr;
    BinaryExpr binary_expr;
} ExprData;

struct Expression {
    ExprType type;
    ExprData expr;
};

const Expression empty_expr = { .type = EXPR_EMPTY };
const Expression quit_expr = { .type = EXPR_QUIT };
const Expression help_expr = { .type = EXPR_HELP };
const Expression invalid_expr = { .type = EXPR_INVALID };

Expression expr_new_var(unsigned char *var_name, Arena *arena) {
    size_t name_len = strnlen((char *)var_name, MAX_INPUT) + 1;
    Expression expr = { .type = EXPR_VAR, .expr = { .var_name = arena_alloc(arena, name_len) }};
    memcpy(expr.expr.var_name, var_name, name_len);
    return expr;
}

Expression expr_new_const(double value) {
    return (Expression) { .type = EXPR_CONSTANT, .expr = { .constant = value }};
}

Expression expr_new_unit_full(Unit unit, Arena *arena) {
    // Duplicate to avoid mismatched unit x expression lifetime
    Unit unit_dup = unit_new(unit.types, unit.degrees, unit.length, arena);
    return (Expression) { .type = EXPR_UNIT, .expr = { .unit = unit_dup }};
}

Expression expr_new_unit(UnitType unit_type, Arena *arena) {
    return expr_new_unit_full(unit_new_single(unit_type, 1, arena), arena);
}

Expression expr_new_neg(Expression right_value, Arena *arena) {
    Expression *right = arena_alloc(arena, sizeof(Expression));
    *right = right_value;
    return (Expression) { .type = EXPR_NEG, .expr = { .unary_expr = { .right = right }}};
}

Expression expr_new_bin(ExprType type, Expression left_value, Expression right_value, Arena *arena) {
    Expression *left = arena_alloc(arena, sizeof(Expression));
    Expression *right = arena_alloc(arena, sizeof(Expression));
    *left = left_value;
    *right = right_value;
    return (Expression) { .type = type, .expr = { .binary_expr = { .left = left, .right = right }}};
}

Expression expr_new_const_unit(double value, Expression unit_expr, Arena *arena) {
    return expr_new_bin(EXPR_CONST_UNIT, expr_new_const(value), unit_expr, arena);
}

Expression expr_new_unit_degree(UnitType unit_type, Expression degree_expr, Arena *arena) {
    return expr_new_bin(EXPR_POW, expr_new_unit(unit_type, arena), degree_expr, arena);
}

Expression expr_new_unit_comp(Expression unit_expr_1, Expression unit_expr_2, Arena *arena) {
    return expr_new_bin(EXPR_COMP_UNIT, unit_expr_1, unit_expr_2, arena);
}

bool expr_is_bin(ExprType type) {
    switch (type) {
        case EXPR_CONSTANT: case EXPR_UNIT: case EXPR_NEG: case EXPR_VAR:
        case EXPR_EMPTY: case EXPR_QUIT: case EXPR_HELP: case EXPR_INVALID:
            return false;
        case EXPR_CONST_UNIT: case EXPR_COMP_UNIT: case EXPR_DIV_UNIT:
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV:
        case EXPR_CONVERT: case EXPR_POW: case EXPR_SET_VAR:
            return true;
    }
}

bool expr_is_unit(ExprType type) {
    switch (type) {
        case EXPR_UNIT: case EXPR_COMP_UNIT: case EXPR_POW: case EXPR_DIV_UNIT:
            return true;
        case EXPR_CONSTANT: case EXPR_NEG: case EXPR_CONST_UNIT: case EXPR_VAR:
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL:
        case EXPR_DIV: case EXPR_CONVERT: case EXPR_SET_VAR: case EXPR_EMPTY:
        case EXPR_QUIT: case EXPR_HELP: case EXPR_INVALID:
            return false;
    }
}

bool expr_is_number(ExprType type) {
    switch (type) {
        case EXPR_CONSTANT: case EXPR_NEG: case EXPR_CONST_UNIT:
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL:
        case EXPR_DIV: case EXPR_CONVERT:
            return true;
        case EXPR_UNIT: case EXPR_COMP_UNIT: case EXPR_POW: case EXPR_SET_VAR:
        case EXPR_VAR: case EXPR_DIV_UNIT: case EXPR_EMPTY: case EXPR_QUIT:
        case EXPR_HELP: case EXPR_INVALID:
            return false;
    }
}

#define EXPR_OP_MAX 13

const char *display_expr_op(ExprType type) {
    switch (type) {
        case EXPR_ADD: return "+";
        case EXPR_SUB: return "-";
        case EXPR_MUL: return "*";
        case EXPR_DIV: return "/";
        case EXPR_DIV_UNIT: return "div unit";
        case EXPR_CONVERT: return "->";
        case EXPR_POW: return "^";
        case EXPR_VAR: return "var";
        case EXPR_SET_VAR: return "set var";
        case EXPR_CONST_UNIT: return "const x unit";
        case EXPR_COMP_UNIT: return "unit x unit";
        case EXPR_NEG: return "negation";
        case EXPR_CONSTANT: return "const";
        case EXPR_UNIT: return "unit";
        case EXPR_EMPTY: return "empty";
        case EXPR_QUIT: return "quit";
        case EXPR_HELP: return "help";
        case EXPR_INVALID: return "invalid";
    }
}

void display_expr(size_t offset, Expression expr, Arena *arena) {
#ifdef DEBUG
    for (size_t i = 0; i < offset; i++) {
        printf("\t");
    }
#endif
    // TODO: reuse display_expr_op here
    if (expr.type == EXPR_CONSTANT) {
        debug("%lf\n", expr.expr.constant);
    } else if (expr.type == EXPR_UNIT) {
        debug("%s\n", display_unit(expr.expr.unit, arena));
    } else if (expr.type == EXPR_VAR) {
        debug("var %s\n", expr.expr.var_name);
    } else if (expr.type == EXPR_NEG) {
        debug("neg\n");
        display_expr(offset + 1, *expr.expr.unary_expr.right, arena);
    } else if (expr.type == EXPR_EMPTY) {
        debug("empty\n");
    } else if (expr.type == EXPR_QUIT) {
        debug("quit\n");
    } else if (expr.type == EXPR_INVALID) {
        debug("invalid\n");
    } else if (expr.type == EXPR_HELP) {
        debug("help\n");
    } else {
        assert(expr.expr.binary_expr.left != NULL);
        assert(expr.expr.binary_expr.right != NULL);
        debug("op: %s\n", display_expr_op(expr.type));
        display_expr(offset + 1, *expr.expr.binary_expr.left, arena);
        display_expr(offset + 1, *expr.expr.binary_expr.right, arena);
    }
}
