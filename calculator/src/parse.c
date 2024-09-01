#pragma once

#include <stdbool.h>
#include "expression.c"
#include "tokenize.c"
#include "arena.c"
#include "debug.c"

bool is_bin_op(TokenType type) {
    switch (type) {
        case TOK_ADD: case TOK_SUB: case TOK_MUL:
        case TOK_DIV: case TOK_CARET: case TOK_CONVERT:
        case TOK_EQUALS:
            return true;
        case TOK_END: case TOK_INVALID: case TOK_QUIT: case TOK_HELP:
        case TOK_NUM: case TOK_VAR: case TOK_WHITESPACE: case TOK_UNIT:
            return false;
    }
}

// Higher number = parse this first, evaluate this last.
int precedence(TokenType op, size_t idx, bool prev_is_bin_op, bool next_is_unit) {
    if (op == TOK_EQUALS) return 10;
    if (op == TOK_CONVERT) return 9;
    if (op == TOK_ADD) return 8;
    if (op == TOK_SUB && idx != 0 && !prev_is_bin_op) return 8; // Not negation
    if (op == TOK_MUL) return 7;
    if (op == TOK_DIV && !next_is_unit) return 7;
    if (op == TOK_SUB && !prev_is_bin_op) return 6; // Normal negation
    // This function will only be called when we are checking
    // for tokens with length > 1, so this signals a constant
    // with something after it.
    if (op == TOK_NUM && idx == 0) return 5;
    if (op == TOK_DIV) return 4;
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
    if (op == TOK_CARET) return true;
    return false;
}

// Only time this should return EXPR_INVALID
// is if we've run into an invalid token
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
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_HELP) {
        debug("help\n");
        return help_expr;
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_UNIT) {
        debug("unit\n");
        return expr_new_unit(tokens.tokens[0].unit_type);
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_NUM) {
        debug("constant\n");
        return expr_new_const(tokens.tokens[0].number);
    }
    if (tokens.length == 1 && tokens.tokens[0].type == TOK_VAR) {
        debug("variable\n");
        return expr_new_var(tokens.tokens[0].var_name, arena);
    }
    if (tokens.length == 1) {
        debug("Invalid expression token length 1\n");
        return invalid_expr;
    }

    // Find the thing we should parse next
    size_t op_idx = 0;
    int best_precedence = 0;
    for (size_t i = 0; i < tokens.length; i++) {
        bool prev_is_bin_op = i == 0 ? false : is_bin_op(tokens.tokens[i - 1].type);
        bool next_is_unit = i == tokens.length - 1 ? false : tokens.tokens[i + 1].type == TOK_UNIT;
        int curr_precedence = precedence(tokens.tokens[i].type, i, prev_is_bin_op, next_is_unit);
        bool curr_left_assoc = left_associative(tokens.tokens[i].type, i, prev_is_bin_op);
        if (curr_precedence > best_precedence || (curr_precedence == best_precedence && curr_left_assoc)) {
            debug("Found higher precedence: %d at idx: %zu\n", tokens.tokens[i].type, i);
            op_idx = i;
            best_precedence = curr_precedence;
        }
    }
    TokenType op = tokens.tokens[op_idx].type;

    if (op == TOK_SUB && op_idx == 0) {
        TokenString right_tokens = (TokenString) { .tokens = tokens.tokens + 1, .length = tokens.length - 1};
        Expression right = parse(right_tokens, arena);
        return expr_new_neg(right, arena);
    }

    ExprType type;
    if (op == TOK_EQUALS) {
        type = EXPR_SET_VAR;
    } else if (op == TOK_CONVERT) {
        type = EXPR_CONVERT;
    } else if (op == TOK_ADD) {
        type = EXPR_ADD;
    } else if (op == TOK_SUB) {
        type = EXPR_SUB;
    } else if (op == TOK_MUL) {
        type = EXPR_MUL;
    } else if (op == TOK_DIV && (op_idx == tokens.length - 1 || tokens.tokens[op_idx + 1].type != TOK_UNIT)) {
        type = EXPR_DIV;
    } else if (op == TOK_DIV) {
        type = EXPR_DIV_UNIT;
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
