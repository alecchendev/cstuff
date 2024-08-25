#pragma once

#include <stdbool.h>
#include <stdio.h>
#include "tokenize.c"
#include "arena.c"
#include "log.c"

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
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_CONVERT,
    EXPR_POW,
    EXPR_EMPTY,
    EXPR_QUIT,
    EXPR_INVALID,
} ExprType;

typedef union {
    double constant;
    UnitType unit_type;
    UnaryExpr unary_expr;
    BinaryExpr binary_expr;
} ExprData;

struct Expression {
    ExprType type;
    ExprData expr;
};

Expression expr_new_const(double value) {
    return (Expression) { .type = EXPR_CONSTANT, .expr = { .constant = value }};
}

Expression expr_new_unit(UnitType unit_type) {
    return (Expression) { .type = EXPR_UNIT, .expr = { .unit_type = unit_type }};
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
    return expr_new_bin(EXPR_POW, expr_new_unit(unit_type), degree_expr, arena);
}

Expression expr_new_unit_comp(Expression unit_expr_1, Expression unit_expr_2, Arena *arena) {
    return expr_new_bin(EXPR_COMP_UNIT, unit_expr_1, unit_expr_2, arena);
}

bool is_bin_op(TokenType type) {
    return type == TOK_ADD || type == TOK_SUB || type == TOK_MUL ||
            type == TOK_DIV || type == TOK_CARET || type == TOK_CONVERT;
}

// Higher number = parse this first, evaluate this last.
int precedence(TokenType op, size_t idx, bool prev_is_bin_op) {
    if (op == TOK_CONVERT) return 8;
    if (op == TOK_ADD) return 7;
    if (op == TOK_SUB && idx != 0 && !prev_is_bin_op) return 7; // Not negation
    if (op == TOK_MUL || op == TOK_DIV) return 6;
    if (op == TOK_SUB && !prev_is_bin_op) return 5; // Normal negation
    // This function will only be called when we are checking
    // for tokens with length > 1, so this signals a constant
    // with something after it.
    if (op == TOK_NUM && idx == 0) return 4;
    if (op == TOK_UNIT && idx != 0) return 3;
    if (op == TOK_CARET) return 2;
    if (op == TOK_SUB) return 1; // Negation within degree
    return 0;
}

// True = break precendence ties by taking the last instance seen
// i.e. evaluate from left to right
bool left_associative(TokenType op, size_t idx, bool prev_is_bin_op) {
    if (op == TOK_ADD) return true;
    if (op == TOK_SUB && idx != 0 && !prev_is_bin_op) return true;
    if (op == TOK_MUL || op == TOK_DIV) return true;
    if (op == TOK_NUM) return false;
    if (op == TOK_UNIT && idx != 0) return true;
    return false;
}

const Expression empty_expr = { .type = EXPR_EMPTY };
const Expression quit_expr = { .type = EXPR_QUIT };
const Expression invalid_expr = { .type = EXPR_INVALID };

Expression parse(TokenString tokens, Arena *arena) {
    debug("Parsing expression\n");
    for (size_t i = 0; i < tokens.length; i++) {
        token_display(tokens.tokens[i]);
    }
    if (tokens.length == 0) {
        debug("empty\n");
        return empty_expr;
    }
    if (tokens.length > 0 && tokens.tokens[tokens.length - 1].type == TOK_END) {
        return parse((TokenString) { .tokens = tokens.tokens, .length = tokens.length - 1}, arena);
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_QUIT) {
        debug("quit\n");
        return quit_expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_UNIT) {
        debug("unit\n");
        return expr_new_unit(tokens.tokens[0].unit_type);
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_NUM) {
        debug("constant\n");
        return expr_new_const(tokens.tokens[0].number);
    }
    if (tokens.length == 1) {
        debug("Invalid expression token length 1\n");
        return invalid_expr;
    }

    // Find the thing we should parse next
    size_t op_idx = 0;
    for (size_t i = 0; i < tokens.length; i++) {
        bool prev_is_bin_op = i == 0 ? false : is_bin_op(tokens.tokens[i - 1].type);
        TokenType prev_best_is_bin_op = op_idx == 0 ? false : is_bin_op(tokens.tokens[op_idx - 1].type);
        int curr_precedence = precedence(tokens.tokens[i].type, i, prev_is_bin_op);
        int best_precedence = precedence(tokens.tokens[op_idx].type, op_idx, prev_best_is_bin_op);
        bool curr_left_assoc = left_associative(tokens.tokens[i].type, i, prev_is_bin_op);
        if (curr_precedence > best_precedence || (curr_precedence == best_precedence && curr_left_assoc)) {
            debug("Found higher precedence: %d at idx: %zu\n", tokens.tokens[i].type, i);
            op_idx = i;
        }
    }
    TokenType op = tokens.tokens[op_idx].type;

    if (op == TOK_SUB && op_idx == 0) {
        TokenString right_tokens = (TokenString) { .tokens = tokens.tokens + 1, .length = tokens.length - 1};
        Expression right = parse(right_tokens, arena);
        return expr_new_neg(right, arena);
    }

    ExprType type;
    if (op == TOK_CONVERT) {
        type = EXPR_CONVERT;
    } else if (op == TOK_ADD) {
        type = EXPR_ADD;
    } else if (op == TOK_SUB) {
        type = EXPR_SUB;
    } else if (op == TOK_MUL) {
        type = EXPR_MUL;
    } else if (op == TOK_DIV) {
        type = EXPR_DIV;
    } else if (op == TOK_NUM) {
        type = EXPR_CONST_UNIT;
    } else if (op == TOK_UNIT) {
        type = EXPR_COMP_UNIT;
    } else if (op == TOK_CARET) {
        type = EXPR_POW;
    } else {
        return invalid_expr;
    }

    // Most binary expression omit the current token,
    // but for some we want to keep the current token.
    size_t left_end_idx = op_idx; // Up until (not including) here
    size_t right_start_idx = op_idx + 1;
    if (type == EXPR_CONST_UNIT) {
        left_end_idx = op_idx + 1;
    } else if (type == EXPR_COMP_UNIT) {
        right_start_idx = op_idx;
    }
    // Assert idxs are within 0 and tokens.length
    TokenString left_tokens = { .tokens = tokens.tokens, .length = left_end_idx };
    TokenString right_tokens = { .tokens = tokens.tokens + right_start_idx, .length = tokens.length - right_start_idx };
    Expression left = parse(left_tokens, arena);
    Expression right = parse(right_tokens, arena);
    return expr_new_bin(type, left, right, arena);
}

const char *display_expr_op(ExprType type) {
    switch (type) {
        case EXPR_ADD:
            return "+";
        case EXPR_SUB:
            return "-";
        case EXPR_MUL:
            return "*";
        case EXPR_DIV:
            return "/";
        case EXPR_CONVERT:
            return "->";
        case EXPR_POW:
            return "^";
        case EXPR_CONST_UNIT:
            return "const x unit";
        case EXPR_COMP_UNIT:
            return "unit x unit";
        default:
            return "?";
    }
}

void display_expr(size_t offset, Expression expr, Arena *arena) {
#ifdef DEBUG
    for (size_t i = 0; i < offset; i++) {
        printf("\t");
    }
#endif
    if (expr.type == EXPR_CONSTANT) {
        debug("%lf\n", expr.expr.constant);
    } else if (expr.type == EXPR_UNIT) {
        debug("%s\n", unit_strings[expr.expr.unit_type]);
    } else if (expr.type == EXPR_NEG) {
        debug("neg\n");
        display_expr(offset + 1, *expr.expr.unary_expr.right, arena);
    } else if (expr.type == EXPR_EMPTY) {
        debug("empty\n");
    } else if (expr.type == EXPR_QUIT) {
        debug("quit\n");
    } else if (expr.type == EXPR_INVALID) {
        debug("invalid\n");
    } else {
        debug("op: %s\n", display_expr_op(expr.type));
        display_expr(offset + 1, *expr.expr.binary_expr.left, arena);
        display_expr(offset + 1, *expr.expr.binary_expr.right, arena);
    }
}

bool check_valid_expr(Expression expr) {
    bool left_valid = false;
    bool right_valid = false;
    ExprType left_type = EXPR_INVALID;
    ExprType right_type = EXPR_INVALID;
    switch (expr.type) {
        case EXPR_CONSTANT: case EXPR_UNIT:
            return true;
        case EXPR_NEG:
            right_type = expr.expr.unary_expr.right->type;
            return right_type == EXPR_CONSTANT || right_type == EXPR_NEG || right_type == EXPR_CONST_UNIT;
        case EXPR_CONST_UNIT: case EXPR_COMP_UNIT: case EXPR_ADD: case EXPR_SUB:
        case EXPR_MUL: case EXPR_DIV: case EXPR_CONVERT: case EXPR_POW:
            left_type = expr.expr.binary_expr.left->type;
            right_type = expr.expr.binary_expr.right->type;
            left_valid = check_valid_expr(*expr.expr.binary_expr.left);
            right_valid = check_valid_expr(*expr.expr.binary_expr.right);
            break;
        case EXPR_EMPTY:
        case EXPR_QUIT:
            return true;
        case EXPR_INVALID:
            return false;
    }
    switch (expr.type) {
        case EXPR_CONST_UNIT:
            return left_valid && right_valid &&
                (left_type == EXPR_CONSTANT || left_type == EXPR_NEG) &&
                (right_type == EXPR_UNIT || right_type == EXPR_COMP_UNIT || right_type == EXPR_POW);
        case EXPR_COMP_UNIT:
            return left_valid && right_valid &&
                (left_type == EXPR_UNIT || left_type == EXPR_COMP_UNIT || left_type == EXPR_POW) &&
                (right_type == EXPR_UNIT || right_type == EXPR_POW);
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV:
            return left_valid && right_valid &&
                (left_type == EXPR_CONSTANT || left_type == EXPR_CONST_UNIT ||
                 left_type == EXPR_NEG || left_type == EXPR_ADD || left_type == EXPR_SUB ||
                 left_type == EXPR_MUL || left_type == EXPR_DIV) &&
                (right_type == EXPR_CONSTANT || right_type == EXPR_CONST_UNIT ||
                 right_type == EXPR_NEG || right_type == EXPR_ADD || right_type == EXPR_SUB ||
                 right_type == EXPR_MUL || right_type == EXPR_DIV);
        case EXPR_CONVERT:
            return left_valid && right_valid &&
                (right_type == EXPR_UNIT || right_type == EXPR_COMP_UNIT || right_type == EXPR_POW);
        case EXPR_POW:
            return left_valid && right_valid &&
                (left_type == EXPR_UNIT) && (right_type == EXPR_CONSTANT || right_type == EXPR_NEG);
        default:
            return false; // unreachable
    }
}

Unit check_unit(Expression expr, Arena *arena) {
    if (expr.type == EXPR_CONSTANT) {
        debug("constant: %lf\n", expr.expr.constant);
        return unit_new_none(arena);
    } else if (expr.type == EXPR_UNIT) {
        debug("unit: %s\n", unit_strings[expr.expr.unit_type]);
        return unit_new_single(expr.expr.unit_type, 1, arena);
    } else if (expr.type == EXPR_NEG) {
        debug("neg\n");
        return check_unit(*expr.expr.unary_expr.right, arena);
    } else if (expr.type == EXPR_EMPTY || expr.type == EXPR_QUIT || expr.type == EXPR_INVALID) {
        debug("empty, quit, or invalid, no unit: %d\n", expr.type);
        return unit_new_unknown(arena);
    }

    Unit left = check_unit(*expr.expr.binary_expr.left, arena);
    Unit right = check_unit(*expr.expr.binary_expr.right, arena);
    debug("left: %s, right: %s\n", display_unit(left, arena), display_unit(right, arena));

    if (expr.type == EXPR_CONVERT) {
        // check if we are able to convert
        // test with single units first
        if (left.length != 1 || right.length != 1) {
            printf("Cannot convert between composite units\n");
            return unit_new_unknown(arena);
        }
        if (is_unit_none(left) || is_unit_none(right)) {
            printf("Cannot convert to or from no unit\n");
            return unit_new_unknown(arena);
        }
        if (is_unit_unknown(left) || is_unit_unknown(right)) {
            printf("Cannot convert to or from unknown unit\n");
            return unit_new_unknown(arena);
        }
        if (left.degrees[0] != 1 || right.degrees[0] != 1) {
            printf("Cannot convert between units with degrees other than 1\n");
            return unit_new_unknown(arena);
        }
        if (unit_category(left.types[0]) != unit_category(right.types[0])) {
            printf("Cannot convert between different unit categories\n");
            return unit_new_unknown(arena);
        }
        return right;
    }

    if (expr.type == EXPR_POW) {
        if (is_unit_unknown(left) || is_unit_none(left) || left.length != 1 || !is_unit_none(right)) {
            printf("Expected single degree unit ^ constant: %s ^ %s\n", display_unit(left, arena), display_unit(right, arena));
            return unit_new_unknown(arena);
        }
        left.degrees[0] *= expr.expr.binary_expr.right->expr.constant;
        return left;
    }

    Unit unit = unit_new_unknown(arena);
    if (is_unit_none(left)) {
        unit = right;
    } else if (is_unit_none(right)) {
        unit = left;
    } else if (is_unit_unknown(left) || is_unit_unknown(right)) {
        // Case to make sure we don't print our error message again
        unit = unit_new_unknown(arena);
    } else if ((expr.type == EXPR_ADD || expr.type == EXPR_SUB) && units_equal(left, right)) {
        unit = right;
    } else if (expr.type == EXPR_MUL || expr.type == EXPR_COMP_UNIT) {
        unit = unit_combine(left, right, arena);
    } else if (expr.type == EXPR_DIV) {
        for (size_t i = 0; i < right.length; i++) {
            right.degrees[i] *= -1;
        }
        unit = unit_combine(left, right, arena);
    } else {
        printf("Units do not match: %s %s %s\n", display_unit(left, arena),
           display_expr_op(expr.type), display_unit(right, arena));
        unit = unit_new_unknown(arena);
    }
    return unit;
}

double evaluate(Expression expr, Arena *arena) {
    double left = 0;
    double right = 0;
    Unit left_unit, right_unit;
    switch (expr.type) {
        case EXPR_CONSTANT:
            return expr.expr.constant;
        case EXPR_UNIT:
            return 0;
        case EXPR_NEG:
            return -evaluate(*expr.expr.unary_expr.right, arena);
        case EXPR_CONST_UNIT:
            return evaluate(*expr.expr.binary_expr.left, arena);
        case EXPR_COMP_UNIT:
            return 0;
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV:
            left = evaluate(*expr.expr.binary_expr.left, arena);
            right = evaluate(*expr.expr.binary_expr.right, arena);
            break;
        case EXPR_CONVERT:
            // TODO: Return a unit tree from check_unit so we don't have to run this
            // (and allocate memory) again?
            left_unit = check_unit(*expr.expr.binary_expr.left, arena);
            right_unit = check_unit(*expr.expr.binary_expr.right, arena);
            left = evaluate(*expr.expr.binary_expr.left, arena);
            return left * unit_convert(left_unit.types[0], right_unit.types[0]);
        case EXPR_POW: // Pow only means unit degrees for now
        case EXPR_EMPTY:
        case EXPR_QUIT:
        case EXPR_INVALID:
            return 0; // unreachable
    }
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
            return 0; // unreachable
    }
}
