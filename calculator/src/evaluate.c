#pragma once

#include <stdarg.h>
#include <stdio.h>
#include "parse.c"
#include "memory.c"
#include "unit.c"


typedef struct ErrorString ErrorString;
struct ErrorString {
    char *msg;
    size_t len;
};

ErrorString err_empty() {
    return (ErrorString) { .msg = NULL, .len = 0 };
}

// Basically printf for an error string!
ErrorString err_new(Arena *arena, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // Determine required size
    va_list ap_dup;
    va_copy(ap_dup, ap);
    int size = vsnprintf(NULL, 0, fmt, ap_dup);
    va_end(ap_dup);

    if (size < 0) {
        va_end(ap);
        return err_empty();
    }

    // Allocate and write string
    ErrorString err = err_empty();
    err.len = size + 1;
    err.msg = arena_alloc(arena, err.len);

    int ret = vsnprintf(err.msg, err.len, fmt, ap);
    va_end(ap);
    assert(ret != -1);

    return err;
}

void substitute_variables(Expression *expr, Memory mem) {
    if (expr->type == EXPR_VAR && memory_contains_var(mem, expr->expr.var_name)) {
        debug("Substituting variable: %s\n", expr->expr.var_name);
        *expr = memory_get_var(mem, expr->expr.var_name);
    } else if (expr->type == EXPR_SET_VAR) {
        debug("Substituting variables for set var expr\n");
        substitute_variables(expr->expr.binary_expr.right, mem);
    } else if (expr->type == EXPR_NEG) {
        debug("Substituting variables for negation expr\n");
        substitute_variables(expr->expr.unary_expr.right, mem);
    } else if (expr_is_bin(expr->type)) {
        debug("Substituting variables for binary expr: %s\n", display_expr_op(expr->type));
        substitute_variables(expr->expr.binary_expr.left, mem);
        substitute_variables(expr->expr.binary_expr.right, mem);
    } else {
        debug("No substitution for type: %s\n", display_expr_op(expr->type));
    }
}

// While these are helpful sometimes,
// there are cases where the error requires
// more knowledge of our parse tree. E.g. "1 / km".
// Not sure the best way to communicate things
// like this. TODO: show user the block of tokens
// where they messed up, and it'll probably be
// obvious.
const char invalid_expr_msg[] = "Invalid expression, unknown reason";
const char invalid_neg_msg[] = "Negation expected right side to be constant, negation, or constant with unit, instead got: %s";
const char invalid_const_unit_msg[] = "Expected to combine constant with unit, instead got left: %s right: %s";
const char invalid_div_unit_msg[] = "Expected to divide two units, instead got left: %s right: %s";
const char invalid_comp_unit_msg[] = "Expected to combine two units, instead got left: %s right: %s";
const char invalid_math_msg[] = "Expected to %s two numbers, instead got left: %s right: %s";
const char invalid_pow_msg[] = "Expected to raise unit to degree, instead got left: %s right: %s";
const char invalid_set_var_msg[] = "Expected to set variable, instead got left: %s right: %s";

bool check_valid_expr(Expression expr, ErrorString *err, Arena *arena) {
    if (err->len > 0) return false;
    bool left_valid = false;
    bool right_valid = false;
    ExprType left_type = EXPR_INVALID;
    ExprType right_type = EXPR_INVALID;
    switch (expr.type) {
        case EXPR_CONSTANT: case EXPR_UNIT: case EXPR_VAR:
            return true;
        case EXPR_NEG:
            right_type = expr.expr.unary_expr.right->type;
            if (right_type == EXPR_CONSTANT || right_type == EXPR_NEG || right_type == EXPR_CONST_UNIT) {
                return true;
            }
            *err = err_new(arena, invalid_neg_msg, display_expr_op(right_type));
            return false;
        case EXPR_CONST_UNIT: case EXPR_COMP_UNIT: case EXPR_ADD: case EXPR_SUB:
        case EXPR_MUL: case EXPR_DIV: case EXPR_CONVERT: case EXPR_POW: case EXPR_DIV_UNIT:
        case EXPR_SET_VAR:
            left_type = expr.expr.binary_expr.left->type;
            right_type = expr.expr.binary_expr.right->type;
            debug("curr: %d left: %d right: %d\n", expr.type, left_type, right_type);
            left_valid = check_valid_expr(*expr.expr.binary_expr.left, err, arena);
            right_valid = check_valid_expr(*expr.expr.binary_expr.right, err, arena);
            break;
        case EXPR_EMPTY: case EXPR_QUIT: case EXPR_HELP:
            return true;
        case EXPR_INVALID:
            *err = err_new(arena, invalid_expr_msg);
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
            if ((left_type == EXPR_CONSTANT || left_type == EXPR_CONST_UNIT ||
                left_type == EXPR_NEG) && (expr_is_unit(right_type))) {
                return true;
            }
            *err = err_new(arena, invalid_const_unit_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_DIV_UNIT:
            if (expr_is_unit(left_type) && expr_is_unit(right_type)) {
                return true;
            }
            *err = err_new(arena, invalid_div_unit_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_COMP_UNIT:
            if ((expr_is_unit(left_type)) &&
                (right_type == EXPR_UNIT || right_type == EXPR_POW)) {
                return true;
            }
            *err = err_new(arena, invalid_comp_unit_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV:
            if ((expr_is_number(left_type) && expr_is_number(right_type)) ||
                (expr.type == EXPR_DIV && expr_is_unit(left_type) && expr_is_unit(right_type))) {
                return true;
            }
            *err = err_new(arena, invalid_math_msg, display_expr_op(expr.type),
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_CONVERT:
            if (expr_is_number(left_type) && expr_is_unit(right_type)) {
                return true;
            }
            *err = err_new(arena, invalid_math_msg, display_expr_op(expr.type),
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_POW:
            if (expr_is_unit(left_type) && (right_type == EXPR_CONSTANT
                || right_type == EXPR_NEG || right_type == EXPR_CONST_UNIT)) {
                return true;
            }
            *err = err_new(arena, invalid_pow_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        case EXPR_SET_VAR:
            if (left_type == EXPR_VAR && (expr_is_number(right_type) ||
                expr_is_unit(right_type))) {
                return true;
            }
            *err = err_new(arena, invalid_set_var_msg,
                display_expr_op(left_type), display_expr_op(right_type));
            return false;
        default:
            assert(false);
            return false;
    }
}

const char none_convert_msg[] = "Convert invalid: one unit is none but not the other: From: %s To: %s";
const char unknown_convert_msg[] = "Convert invalid, unknown units: From: %s To: %s";
const char neq_length_msg[] = "Convert invalid: lengths not equal: From: %s To: %s";
const char general_bad_convert_msg[] = "Convert invalid: From: %s To %s";

bool unit_convert_valid(Unit a, Unit b, ErrorString *err, Arena *arena) {
    if (is_unit_none(a) != is_unit_none(b)) {
        *err = err_new(arena, none_convert_msg,
            display_unit(a, arena), display_unit(b, arena));
        return false;
    }
    if (is_unit_unknown(a) || is_unit_unknown(b)) {
        return false;
        *err = err_new(arena, unknown_convert_msg,
            display_unit(a, arena), display_unit(b, arena));
        return false;
    }
    if (a.length != b.length) {
        // Just reusing instead of a separate double const
        *err = err_new(arena, neq_length_msg,
            display_unit(a, arena), display_unit(b, arena));
        return false;
    }
    bool all_convertible = true;
    for (size_t i = 0; i < a.length; i++) {
        bool convertible = false;
        for (size_t j = 0; j < b.length; j++) {
            if (unit_category(a.types[i]) == unit_category(b.types[j]) && a.degrees[i] == b.degrees[j]) {
                convertible = true;
                break;
            }
        }
        all_convertible &= convertible;
    }
    if (!all_convertible) {
        *err = err_new(arena, general_bad_convert_msg,
            display_unit(a, arena), display_unit(b, arena));
    }
    return all_convertible;
}

double evaluate(Expression expr, Memory mem, ErrorString *err, Arena *arena);

Unit check_unit(Expression expr, Memory mem, ErrorString *err, Arena *arena) {
    if (expr.type == EXPR_CONSTANT) {
        debug("constant: %lf\n", expr.expr.constant);
        return unit_new_none(arena);
    } else if (expr.type == EXPR_UNIT) {
        debug("unit: %s\n", display_unit(expr.expr.unit, arena));
        return expr.expr.unit;
    } else if (expr.type == EXPR_VAR) {
        debug("var: %s\n", expr.expr.var_name);
        if (!memory_contains_var(mem, expr.expr.var_name)) {
            *err = err_new(arena, "Variable not defined: %s", expr.expr.var_name);
            return unit_new_unknown(arena);
        }
        Expression var_expr = memory_get_var(mem, expr.expr.var_name);
        return check_unit(var_expr, mem, err, arena);
    } else if (expr.type == EXPR_NEG) {
        debug("neg\n");
        return check_unit(*expr.expr.unary_expr.right, mem, err, arena);
    } else if (expr.type == EXPR_EMPTY || expr.type == EXPR_QUIT || expr.type == EXPR_INVALID) {
        debug("empty, quit, or invalid, no unit: %d\n", expr.type);
        return unit_new_unknown(arena);
    }

    Unit left = check_unit(*expr.expr.binary_expr.left, mem, err, arena);
    Unit right = check_unit(*expr.expr.binary_expr.right, mem, err, arena);
    debug("left: %s, right: %s\n", display_unit(left, arena), display_unit(right, arena));

    Unit unit = unit_new_unknown(arena);
    if (is_unit_unknown(left) || is_unit_unknown(right)) {
        // Already printed reason to err when checking unit of left/right
        debug("unit unknown\n");
        unit = unit_new_unknown(arena);
    } else if (expr.type == EXPR_POW) {
        if (is_unit_none(left) || !is_unit_none(right)) {
            *err = err_new(arena, "Expected single degree unit ^ constant: %s ^ %s",
                display_unit(left, arena), display_unit(right, arena));
            return unit_new_unknown(arena);
        }
        debug("pow: %s ^ %lf\n", display_unit(left, arena), expr.expr.binary_expr.right->expr.constant);
        Unit left_dup = unit_new(left.types, left.degrees, left.length, arena);
        double degree = evaluate(*expr.expr.binary_expr.right, mem, err, arena);
        for (size_t i = 0; i < left_dup.length; i++) {
            left_dup.degrees[i] *= degree;
        }
        unit = left_dup;
    } else if (expr.type == EXPR_CONVERT) {
        if (!unit_convert_valid(left, right, err, arena)) {
            return unit_new_unknown(arena);
        }
        unit = right;
    } else if (expr.type == EXPR_ADD || expr.type == EXPR_SUB) {
        if (unit_convert_valid(right, left, err, arena)) {
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
    } else if (expr.type == EXPR_COMP_UNIT || expr.type == EXPR_CONST_UNIT) {
        debug("combining units for comp\n");
        unit = unit_combine(left, right, true, arena);
        if (is_unit_unknown(unit)) {
            *err = err_new(arena, "Cannot compose units of same category: Left: %s Right: %s",
                display_unit(left, arena), display_unit(right, arena));
        }
    } else if (expr.type == EXPR_DIV || expr.type == EXPR_DIV_UNIT) {
        debug("dividing units\n");
        // TODO: reject div unit for same category
        Unit right_dup = unit_new(right.types, right.degrees, right.length, arena);
        for (size_t i = 0; i < right_dup.length; i++) {
            right_dup.degrees[i] *= -1;
        }
        unit = unit_combine(left, right_dup, false, arena);
    } else {
        *err = err_new(arena, "Units do not match: %s %s %s", display_unit(left, arena),
           display_expr_op(expr.type), display_unit(right, arena));
        unit = unit_new_unknown(arena);
    }
    return unit;
}

double evaluate(Expression expr, Memory mem, ErrorString *err, Arena *arena) {
    double left = 0;
    double right = 0;
    Unit left_unit, right_unit;
    switch (expr.type) {
        case EXPR_CONSTANT:
            return expr.expr.constant;
        case EXPR_VAR:
            return evaluate(memory_get_var(mem, expr.expr.var_name), mem, err, arena);
        case EXPR_POW: // Pow only means unit degrees for now
        case EXPR_UNIT:
        case EXPR_COMP_UNIT:
        case EXPR_DIV_UNIT:
            return 0;
        case EXPR_NEG:
            return -evaluate(*expr.expr.unary_expr.right, mem, err, arena);
        case EXPR_CONST_UNIT:
            return evaluate(*expr.expr.binary_expr.left, mem, err, arena);
        case EXPR_SET_VAR:
            assert(false);
        case EXPR_ADD: case EXPR_SUB: case EXPR_MUL: case EXPR_DIV:
            left_unit = check_unit(*expr.expr.binary_expr.left, mem, err, arena);
            right_unit = check_unit(*expr.expr.binary_expr.right, mem, err, arena);
            left = evaluate(*expr.expr.binary_expr.left, mem, err, arena);
            right = evaluate(*expr.expr.binary_expr.right, mem, err, arena);
            break;
        case EXPR_CONVERT:
            // TODO: Return a unit tree from check_unit so we don't have to run this
            // (and allocate memory) again?
            left_unit = check_unit(*expr.expr.binary_expr.left, mem, err, arena);
            right_unit = check_unit(*expr.expr.binary_expr.right, mem, err, arena);
            left = evaluate(*expr.expr.binary_expr.left, mem, err, arena);
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
            if (right == 0) {
                *err = err_new(arena, "Cannot divide by zero");
                return 0;
            }
            return left / right;
        default:
            assert(false);
            return 0;
    }
}
