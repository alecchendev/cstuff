#pragma once

#include "parse.c"
#include "unit.c"

double evaluate(Expression expr, Arena *arena);

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
            debug("curr: %d left: %d right: %d\n", expr.type, left_type, right_type);
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
        debug("pow: %s ^ %lf\n", display_unit(left, arena), expr.expr.binary_expr.right->expr.constant);
        left.degrees[0] *= evaluate(*expr.expr.binary_expr.right, arena);
        return left;
    }

    Unit unit = unit_new_unknown(arena);
    if (is_unit_none(left)) {
        debug("unit left none\n");
        unit = right;
    } else if (is_unit_none(right)) {
        debug("unit right none\n");
        unit = left;
    } else if (is_unit_unknown(left) || is_unit_unknown(right)) {
        // Case to make sure we don't print our error message again
        debug("unit unknown\n");
        unit = unit_new_unknown(arena);
    } else if ((expr.type == EXPR_ADD || expr.type == EXPR_SUB) && units_equal(left, right)) {
        debug("units equal\n");
        unit = right;
    } else if (expr.type == EXPR_MUL || expr.type == EXPR_COMP_UNIT) {
        debug("combining units\n");
        unit = unit_combine(left, right, arena);
    } else if (expr.type == EXPR_DIV) {
        debug("dividing units\n");
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
            return left * unit_conversion[left_unit.types[0]][right_unit.types[0]];
        case EXPR_POW: // Pow only means unit degrees for now
        case EXPR_EMPTY:
        case EXPR_QUIT:
        case EXPR_INVALID:
            assert(false);
            return 0;
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
            assert(false);
            return 0;
    }
}
