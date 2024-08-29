#pragma once

#include "parse.c"
#include "unit.c"
#include <stdio.h>

double evaluate(Expression expr, Arena *arena);

typedef struct ErrorString ErrorString;
struct ErrorString {
    char *msg;
    size_t len;
};

ErrorString err_empty() {
    return (ErrorString) { .msg = NULL, .len = 0 };
}

// While these are helpful sometimes,
// there are cases where the error requires
// more knowledge of our parse tree. E.g. "1 / km".
// Not sure the best way to communicate things
// like this. TODO: show user the block of tokens
// where they messed up, and it'll probably be
// obvious.
const char invalid_token_msg[] = "Invalid token";
const char invalid_neg_msg[] = "Negation expected right side to be constant, negation, or constant with unit, instead got: %s";
const char invalid_const_unit_msg[] = "Expected to combine constant with unit, instead got left: %s right: %s";
const char invalid_div_unit_msg[] = "Expected to divide two units, instead got left: %s right: %s";
const char invalid_comp_unit_msg[] = "Expected to combine two units, instead got left: %s right: %s";
const char invalid_math_msg[] = "Expected to %s two numbers, instead got left: %s right: %s";
const char invalid_pow_msg[] = "Expected to raise unit to degree, instead got left: %s right: %s";

bool check_valid_expr(Expression expr, ErrorString *err, Arena *arena) {
    if (err->len > 0) return false;
    bool left_valid = false;
    bool right_valid = false;
    ExprType left_type = EXPR_INVALID;
    ExprType right_type = EXPR_INVALID;
    switch (expr.type) {
        case EXPR_CONSTANT: case EXPR_UNIT:
            return true;
        case EXPR_NEG:
            right_type = expr.expr.unary_expr.right->type;
            if (right_type == EXPR_CONSTANT || right_type == EXPR_NEG || right_type == EXPR_CONST_UNIT) {
                return true;
            }
            err->len = sizeof(invalid_neg_msg) + EXPR_OP_MAX;
            err->msg = arena_alloc(arena, err->len);
            snprintf(err->msg, err->len, invalid_neg_msg, display_expr_op(right_type));
            return false;
        case EXPR_CONST_UNIT: case EXPR_COMP_UNIT: case EXPR_ADD: case EXPR_SUB:
        case EXPR_MUL: case EXPR_DIV: case EXPR_CONVERT: case EXPR_POW: case EXPR_DIV_UNIT:
            left_type = expr.expr.binary_expr.left->type;
            right_type = expr.expr.binary_expr.right->type;
            debug("curr: %d left: %d right: %d\n", expr.type, left_type, right_type);
            left_valid = check_valid_expr(*expr.expr.binary_expr.left, err, arena);
            right_valid = check_valid_expr(*expr.expr.binary_expr.right, err, arena);
            break;
        case EXPR_EMPTY: case EXPR_QUIT: case EXPR_HELP:
            return true;
        case EXPR_INVALID:
            err->len = sizeof(invalid_token_msg);
            err->msg = arena_alloc(arena, err->len);
            memcpy(err->msg, invalid_token_msg, err->len); // TODO: print actual token
            return false;
    }
    if (!left_valid || !right_valid) {
        debug("Left or right invalid: Left: %d Right: %d\n", left_valid, right_valid);
        return false;
    }
    debug("Curr type: %s Left: %s Right %s\n",
          display_expr_op(expr.type), display_expr_op(left_type), display_expr_op(right_type));
    switch (expr.type) {
        case EXPR_CONST_UNIT:
            if ((left_type == EXPR_CONSTANT || left_type == EXPR_NEG) &&
                (expr_is_unit(right_type))) {
                return true;
            }
            err->len = sizeof(invalid_const_unit_msg) + 2 * EXPR_OP_MAX;
            err->msg = arena_alloc(arena, err->len);
            snprintf(err->msg, err->len, invalid_const_unit_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_DIV_UNIT:
            if (expr_is_unit(left_type) && expr_is_unit(right_type)) {
                return true;
            }
            err->len = sizeof(invalid_div_unit_msg) + 2 * EXPR_OP_MAX;
            err->msg = arena_alloc(arena, err->len);
            snprintf(err->msg, err->len, invalid_div_unit_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_COMP_UNIT:
            if ((expr_is_unit(left_type)) &&
                (right_type == EXPR_UNIT || right_type == EXPR_POW)) {
                return true;
            }
            err->len = sizeof(invalid_comp_unit_msg) + 2 * EXPR_OP_MAX;
            err->msg = arena_alloc(arena, err->len);
            snprintf(err->msg, err->len, invalid_comp_unit_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV:
            if ((expr_is_number(left_type) && expr_is_number(right_type)) ||
                (expr.type == EXPR_DIV && expr_is_unit(left_type) && expr_is_unit(right_type))) {
                return true;
            }
            err->len = sizeof(invalid_math_msg) + 2 * EXPR_OP_MAX;
            err->msg = arena_alloc(arena, err->len);
            snprintf(err->msg, err->len, invalid_math_msg, display_expr_op(expr.type),
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_CONVERT:
            if (expr_is_number(left_type) && expr_is_unit(right_type)) {
                return true;
            }
            err->len = sizeof(invalid_math_msg) + 2 * EXPR_OP_MAX;
            err->msg = arena_alloc(arena, err->len);
            snprintf(err->msg, err->len, invalid_math_msg, display_expr_op(expr.type),
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_POW:
            if (expr_is_unit(left_type) && (right_type == EXPR_CONSTANT || right_type == EXPR_NEG)) {
                return true;
            }
            err->len = sizeof(invalid_pow_msg) + 2 * EXPR_OP_MAX;
            err->msg = arena_alloc(arena, err->len);
            snprintf(err->msg, err->len, invalid_pow_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        default:
            assert(false);
            return false;
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
        if (is_unit_unknown(left) || is_unit_unknown(right)) {
            printf("Cannot convert to or from unknown unit\n");
            return unit_new_unknown(arena);
        }
        if (!unit_convert_valid(left, right, arena)) {
            // Already printed reason inside of function
            return unit_new_unknown(arena);
        }
        return right;
    }

    if (expr.type == EXPR_POW) {
        if (is_unit_unknown(left) || is_unit_none(left) || left.length != 1 || !is_unit_none(right)) {
            printf("Expected single degree unit ^ constant: %s ^ %s\n", display_unit(left, arena), display_unit(right, arena));
            return unit_new_unknown(arena);
        }
        debug("pow: %s ^ %lf\n", display_unit(left, arena), expr.expr.binary_expr.right->expr.constant);
        left.degrees[0] *= evaluate(*expr.expr.binary_expr.right, arena);
        return left;
    }

    Unit unit = unit_new_unknown(arena);
    if (is_unit_unknown(left) || is_unit_unknown(right)) {
        // Case to make sure we don't print our error message again
        debug("unit unknown\n");
        unit = unit_new_unknown(arena);
    } else if (expr.type == EXPR_ADD || expr.type == EXPR_SUB) {
        if (unit_convert_valid(right, left, arena)) {
            debug("units convertible for add/sub\n");
            unit = left;
        } else {
            debug("units not convertible for add/sub\n");
            unit = unit_new_unknown(arena);
        }
    } else if (is_unit_none(left)) {
        debug("unit left none\n");
        unit = right;
    } else if (is_unit_none(right)) {
        debug("unit right none\n");
        unit = left;
    } else if (expr.type == EXPR_MUL) {
        debug("combining units for mul\n");
        unit = unit_combine(left, right, false, arena);
    } else if (expr.type == EXPR_COMP_UNIT) {
        debug("combining units for comp\n");
        unit = unit_combine(left, right, true, arena);
    } else if (expr.type == EXPR_DIV || expr.type == EXPR_DIV_UNIT) {
        debug("dividing units\n");
        for (size_t i = 0; i < right.length; i++) {
            right.degrees[i] *= -1;
        }
        unit = unit_combine(left, right, false, arena);
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
        case EXPR_POW: // Pow only means unit degrees for now
        case EXPR_UNIT:
        case EXPR_COMP_UNIT:
        case EXPR_DIV_UNIT:
            return 0;
        case EXPR_NEG:
            return -evaluate(*expr.expr.unary_expr.right, arena);
        case EXPR_CONST_UNIT:
            return evaluate(*expr.expr.binary_expr.left, arena);
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV:
            left_unit = check_unit(*expr.expr.binary_expr.left, arena);
            right_unit = check_unit(*expr.expr.binary_expr.right, arena);
            left = evaluate(*expr.expr.binary_expr.left, arena);
            right = evaluate(*expr.expr.binary_expr.right, arena);
            break;
        case EXPR_CONVERT:
            // TODO: Return a unit tree from check_unit so we don't have to run this
            // (and allocate memory) again?
            left_unit = check_unit(*expr.expr.binary_expr.left, arena);
            right_unit = check_unit(*expr.expr.binary_expr.right, arena);
            left = evaluate(*expr.expr.binary_expr.left, arena);
            return left * unit_convert_factor(left_unit, right_unit, arena);
        case EXPR_EMPTY: case EXPR_QUIT: case EXPR_HELP: case EXPR_INVALID:
            assert(false);
            return 0;
    }
    right *= unit_convert_factor(right_unit, left_unit, arena);
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
            assert(false);
            return 0;
    }
}
