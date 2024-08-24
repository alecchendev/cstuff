#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tokenize.c"
#include "arena.c"
#include "log.c"

typedef struct Expression Expression;
typedef struct Constant Constant;
typedef struct BinaryExpr BinaryExpr;
typedef struct Convert Convert;

struct Constant {
    double value;
    Unit unit;
};

struct BinaryExpr {
    Expression *left;
    Expression *right;
};

typedef enum {
    EXPR_CONSTANT,
    EXPR_UNIT,
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_CONVERT,
    EXPR_EMPTY,
    EXPR_QUIT,
} ExprType;

typedef union {
    Constant constant;
    BinaryExpr binary_expr;
    Unit unit;
} ExprData;

struct Expression {
    ExprType type;
    ExprData expr;
};

// These expression constructors should only be used for tests, otherwise
// we need to make sure the pointers are valid.
Expression *expr_new_const_unit(double value, Unit unit, Arena *arena) {
    Expression *expr = arena_alloc(arena, sizeof(Expression));
    *expr = (Expression) {
        .type = EXPR_CONSTANT,
        .expr = { .constant = { .value = value, .unit = unit } },
    };
    return expr;
}

Expression *expr_new_const(double value, Arena *arena) {
    return expr_new_const_unit(value, unit_new_none(arena), arena);
}

Expression *expr_new_bin(ExprType type, Expression *left, Expression *right, Arena *arena) {
    Expression *expr = arena_alloc(arena, sizeof(Expression));
    *expr = (Expression) {
        .type = type,
        .expr = { .binary_expr = { .left = left, .right = right } },
    };
    return expr;
}

bool is_bin_op(TokenType type) {
    return type == TOK_ADD || type == TOK_SUB || type == TOK_MUL ||
            type == TOK_DIV || type == TOK_CARET || type == TOK_CONVERT;
}

// Precedence - lower number means this should be evaluated first (binds more tightly)
int precedence(TokenType op, TokenType prev_token, size_t index) {
    if (op == TOK_SUB && is_bin_op(prev_token)) return 1;
    if (op == TOK_CARET) return 2;
    if (op == TOK_UNIT && index != 0) return 3;
    if (op == TOK_NUM && index == 0) return 4;
    if (op == TOK_SUB && index == 0) return 5;
    if (op == TOK_MUL || op == TOK_DIV) return 6;
    if (op == TOK_ADD || op == TOK_SUB) return 7;
    if (op == TOK_CONVERT) return 8;
    return 0; // error case, shouldn't matter
}

// True means take the last instance when evaluating equal precedence
bool left_associative(TokenType op, TokenType prev_token, size_t index) {
    if (op == TOK_SUB && is_bin_op(prev_token)) return false;
    if (op == TOK_CARET) return true; // Doesn't apply yet
    if (op == TOK_UNIT && index != 0) return true;
    if (op == TOK_NUM && index == 0) return false;
    if (op == TOK_SUB && index == 0) return false; // Doesn't apply
    if (op == TOK_MUL || op == TOK_DIV) return true;
    if (op == TOK_ADD || op == TOK_SUB) return true;
    if (op == TOK_CONVERT) return true; // Doesn't apply
    return false; // error case, shouldn't matter
}

Expression *parse(TokenString tokens, Arena *arena) {
    debug("Parsing expression\n");
    for (size_t i = 0; i < tokens.length; i++) {
        token_display(tokens.tokens[i]);
    }
    if (tokens.length == 0 || (tokens.length == 1 && tokens.tokens[0].type == TOK_END)) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        *expr = (Expression) { .type = EXPR_EMPTY };
        debug("empty\n");
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_QUIT) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        *expr = (Expression) { .type = EXPR_QUIT };
        debug("quit\n");
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_NUM) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        expr->type = EXPR_CONSTANT;
        expr->expr.constant.value = tokens.tokens[0].number;
        expr->expr.constant.unit = unit_new_none(arena);
        debug("constant\n");
        return expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_UNIT) {
        Expression *expr = arena_alloc(arena, sizeof(Expression));
        expr->type = EXPR_UNIT;
        expr->expr.unit = unit_new_single(tokens.tokens[0].unit_type, 1, arena);
        debug("unit\n");
        return expr;
    }
    if (tokens.length == 1) {
        debug("Invalid expression token length 1\n");
        return NULL;
    }
    if (tokens.tokens[tokens.length - 1].type == TOK_END) {
        debug("Parsing end token\n");
        return parse((TokenString) { .tokens = tokens.tokens, .length = tokens.length - 1 }, arena);
    }

    // Find highest precedence, then last instance if left associative,
    // otherwise the first instance.
    // When we evaluate, we evaluate the leaves first, so
    // when we're creating the root here, we want the thing
    // that will be evaluated last.
    int op_index = 0;
    for (int i = 0; i < tokens.length; i++) {
        TokenType prev_token = i == 0 ? TOK_WHITESPACE : tokens.tokens[i - 1].type;
        TokenType prev_best_token = op_index == 0 ? TOK_WHITESPACE : tokens.tokens[op_index - 1].type;
        int curr_precedence = precedence(tokens.tokens[i].type, prev_token, i);
        int best_precedence = precedence(tokens.tokens[op_index].type, prev_best_token, op_index);
        bool curr_left_assoc = left_associative(tokens.tokens[i].type, prev_token, i);
        if (curr_precedence > best_precedence || (curr_precedence == best_precedence && curr_left_assoc)) {
            debug("Found higher precedence: %d at index: %d\n", tokens.tokens[i].type, i);
            op_index = i;
        }
    }
    TokenType op = tokens.tokens[op_index].type;
    // Maybe don't even need this check?
    if (is_bin_op(op) && ((op_index == 0 && op != TOK_SUB) || op_index == tokens.length - 1)) {
        debug("Binop at beginning or end: tokens.length: %zu op_index: %d op: %d\n", tokens.length, op_index, op);
        return NULL;
    }
    debug("Found token: %d at index: %d\n", op, op_index);
    Expression *expr = arena_alloc(arena, sizeof(Expression));
    if (op == TOK_ADD) {
        expr->type = EXPR_ADD;
    } else if (op == TOK_SUB) {
        expr->type = EXPR_SUB;
    } else if (op == TOK_MUL) {
        expr->type = EXPR_MUL;
    } else if (op == TOK_DIV) {
        expr->type = EXPR_DIV;
    } else if (op == TOK_CARET) {
        expr->type = EXPR_UNIT;
    } else if (op == TOK_UNIT) {
        expr->type = EXPR_UNIT;
    } else if (op == TOK_NUM) {
        expr->type = EXPR_CONSTANT;
    } else {
        expr->type = EXPR_CONVERT;
    }

    if (op == TOK_SUB && op_index == 0) {
        TokenString right_tokens = { .tokens = tokens.tokens + 1, .length = tokens.length - 1 };
        Expression *right = parse(right_tokens, arena);
        if (right == NULL || right->type != EXPR_CONSTANT) {
            debug("expected constant after negation\n");
            return NULL;
        }
        right->expr.constant.value *= -1;
        debug("negated constant\n");
        return right;
    }

    if (op == TOK_UNIT) {
        // TODO: somehow simplify this into the binary expression stuff? maybe like
        // the caret case as well
        TokenString left_tokens = { .tokens = tokens.tokens, .length = op_index };
        Expression *left = parse(left_tokens, arena);
        if (left == NULL || left->type == EXPR_EMPTY || left->type == EXPR_QUIT) {
            return left;
        }
        if (left->type != EXPR_UNIT) {
            debug("expected unit before unit\n");
            return NULL;
        }
        TokenString right_tokens = { .tokens = tokens.tokens + op_index, .length = tokens.length - op_index };
        Expression *right = parse(right_tokens, arena);
        if (right == NULL || right->type == EXPR_EMPTY || right->type == EXPR_QUIT) {
            return right;
        }
        if (right->type != EXPR_UNIT) {
            debug("expected unit after unit\n");
            return NULL;
        }
        expr->expr.unit = unit_combine(left->expr.unit, right->expr.unit, arena);
        return expr;
    }

    if (op == TOK_NUM) {
        TokenString right_tokens = { .tokens = tokens.tokens + op_index + 1, .length = tokens.length - op_index - 1 };
        Expression *right = parse(right_tokens, arena);
        if (right == NULL || (right->type != EXPR_UNIT && right->type != EXPR_EMPTY)) {
            debug("expected unit after number\n");
            return NULL;
        }
        expr->expr.constant.unit = right->type == EXPR_EMPTY ? unit_new_none(arena) : right->expr.unit;
        expr->expr.constant.value = tokens.tokens[op_index].number;
        debug("number with unit\n");
        return expr;
    }

    TokenString left_tokens = { .tokens = tokens.tokens, .length = op_index };
    Expression *left = parse(left_tokens, arena);
    if (left == NULL || left->type == EXPR_EMPTY || left->type == EXPR_QUIT) {
        return left;
    }
    TokenString right_tokens = { .tokens = tokens.tokens + op_index + 1, .length = tokens.length - op_index - 1 };
    Expression *right = parse(right_tokens, arena);
    if (right == NULL || right->type == EXPR_EMPTY || right->type == EXPR_QUIT) {
        return right;
    }

    // Maybe not necessary, but we simplify the unit here...?
    // We could just reject wrong expression types and use a binary expression...TODO
    if (op == TOK_CARET) {
        if (left->type != EXPR_UNIT || right->type != EXPR_CONSTANT) {
            debug("Trying to apply degree to non-unit, or degree is non-constant\n");
            return NULL;
        }
        if (left->expr.unit.length != 1 || left->expr.unit.degrees[0] != 1) {
            debug("Can only can apply caret to single units with no degrees\n");
            return NULL;
        }
        if (!is_unit_none(right->expr.constant.unit) && !is_unit_unknown(right->expr.constant.unit)) {
            debug("Degree somehow has a unit\n");
            return NULL;
        }
        expr->expr.unit = unit_new_single(left->expr.unit.types[0], right->expr.constant.value, arena);
        debug("unit with degree: unit: %s\n", display_unit(expr->expr.unit, arena));
        return expr;
    }

    if (expr->type == EXPR_CONVERT && right->type != EXPR_UNIT) {
        return NULL;
    }

    expr->expr.binary_expr.left = left;
    expr->expr.binary_expr.right = right;

    return expr;
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
        default:
            return "?";
    }
}

void display_expr(size_t offset, Expression expr, Arena *arena) {
    for (size_t i = 0; i < offset; i++) {
        debug("\t");
    }
    if (expr.type == EXPR_CONSTANT) {
        debug("%lf %s\n", expr.expr.constant.value, display_unit(expr.expr.constant.unit, arena));
    } else if (expr.type == EXPR_UNIT) {
        debug("%s\n", display_unit(expr.expr.unit, arena));
    } else if (expr.type == EXPR_EMPTY) {
        debug("empty");
    } else if (expr.type == EXPR_QUIT) {
        debug("quit");
    } else {
        debug("op: %s\n", display_expr_op(expr.type));
        display_expr(offset + 1, *expr.expr.binary_expr.left, arena);
        display_expr(offset + 1, *expr.expr.binary_expr.right, arena);
    }
}

Unit check_unit(Expression expr, Arena *arena) {
    if (expr.type == EXPR_CONSTANT) {
        debug("constant unit: %s\n", display_unit(expr.expr.constant.unit, arena));
        return expr.expr.constant.unit;
    } else if (expr.type == EXPR_UNIT) {
        debug("unit: %s\n", display_unit(expr.expr.unit, arena));
        return expr.expr.unit;
    } else if (expr.type == EXPR_EMPTY || expr.type == EXPR_QUIT) {
        debug("empty or quit, no unit\n");
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
    } else if (expr.type == EXPR_MUL) {
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
    if (expr.type == EXPR_CONSTANT) {
        return expr.expr.constant.value;
    } else if (expr.type == EXPR_UNIT) {
        return 0;
    }
    double left = evaluate(*expr.expr.binary_expr.left, arena);
    double right = evaluate(*expr.expr.binary_expr.right, arena);
    Unit left_unit;
    Unit right_unit;
    switch (expr.type) {
        case EXPR_ADD:
            return left + right;
        case EXPR_SUB:
            return left - right;
        case EXPR_MUL:
            return left * right;
        case EXPR_DIV:
            return left / right;
        case EXPR_CONVERT:
            // TODO: Return a unit tree from check_unit so we don't have to run this
            // (and allocate memory) again?
            left_unit = check_unit(*expr.expr.binary_expr.left, arena);
            right_unit = check_unit(*expr.expr.binary_expr.right, arena);
            return left * unit_convert(left_unit.types[0], right_unit.types[0]);
        default:
            exit(1); // TODO
    }
}
