#pragma once

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "arena.c"
#include "unit.c"
#include "debug.c"

typedef enum TokenType TokenType;
enum TokenType {
    TOK_NUM,
    TOK_UNIT,
    TOK_VAR,
    TOK_EQUALS,
    TOK_CONVERT,
    TOK_ADD,
    TOK_SUB,
    TOK_MUL,
    TOK_DIV,
    TOK_CARET,
    TOK_WHITESPACE,
    TOK_HELP,
    TOK_QUIT,
    TOK_END,
    TOK_INVALID,
};

typedef struct Token Token;
struct Token {
    TokenType type;
    union {
        UnitType unit_type;
        double number;
        unsigned char *var_name;
    };
};

#define MAX_INPUT 256

bool char_in_set(char c, const unsigned char *set, size_t set_length) {
    for (size_t i = 0; i < set_length; i++) {
        if (c == set[i]) {
            return true;
        }
    }
    return false;
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

double char_to_digit(char c) {
    return c - '0';
}

bool is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

const Token invalid_token = {TOK_INVALID};
const Token end_token = {TOK_END};
const Token quit_token = {TOK_QUIT};
const Token whitespace_token = {TOK_WHITESPACE};
const Token help_token = {TOK_HELP};
const Token add_token = {TOK_ADD};
const Token sub_token = {TOK_SUB};
const Token mul_token = {TOK_MUL};
const Token div_token = {TOK_DIV};
const Token caret_token = {TOK_CARET};
const Token convert_token = {TOK_CONVERT};
const Token equals_token = {TOK_EQUALS};

Token token_new_num(double num) {
    return (Token){TOK_NUM, .number = num };
}

Token token_new_unit(UnitType unit) {
    return (Token){TOK_UNIT, .unit_type = unit};
}

Token token_new_variable(char string_token[MAX_INPUT], Arena *arena) {
    size_t name_len = strnlen(string_token, MAX_INPUT) + 1;
    Token token = { .type = TOK_VAR, .var_name = arena_alloc(arena, name_len) };
    memcpy(token.var_name, string_token, name_len);
    return token;
}

// TODO: make this more generic where I can simply define
// basically a table of strings and their corresponding tokens
Token next_token(const char *input, size_t *pos, size_t length, Arena *arena) {
    if (*pos >= length || input[*pos] == '\0') {
        if (*pos != length) {
            debug("Invalid end of input, next: %c\n", input[*pos+1]);
            return invalid_token;
        }
        debug("End of input, next: %c\n", input[*pos+1]);
        return end_token;
    }

    if (is_letter(input[*pos])) {
        debug("Letter: %c, next: %c\n", input[*pos], input[*pos+1]);
        char string_token[MAX_INPUT] = {0};
        for (size_t i = 0; is_letter(input[*pos]) || is_digit(input[*pos]) || input[*pos] == '_'; i++) {
            string_token[i] = input[*pos];
            (*pos)++;
        }
        if (strnlen(string_token, 5) == 4
            && (strncmp(string_token, "quit", 4) == 0
                || strncmp(string_token, "exit", 4) == 0)) {
            return quit_token;
        }
        if (strnlen(string_token, 5) == 4 && strncmp(string_token, "help", 4) == 0) {
            return help_token;
        }
        for (size_t unit = 0; unit < UNIT_COUNT; unit++) {
            const char *unit_str = unit_strings[unit];
            const size_t len = strnlen(unit_str, MAX_UNIT_STRING + 1);
            const size_t str_len = strnlen(string_token, MAX_UNIT_STRING + 1);
            if (str_len == len && strncmp(string_token, unit_str, len + 1) == 0) {
                return token_new_unit(unit);
            }
        }
        return token_new_variable(string_token, arena);
    }

    const unsigned char whitespace[3] = {' ', '\t', '\n'};
    if (char_in_set(input[*pos], whitespace, sizeof(whitespace))) {
        debug("Whitespace, next: %c\n", input[*pos+1]);
        while (char_in_set(input[*pos], whitespace, sizeof(whitespace))) {
            (*pos)++;
        }
        return whitespace_token;
    }

    const unsigned char operators[6] = {'+', '-', '*', '/', '^', '='};
    if (char_in_set(input[*pos], operators, sizeof(operators))) {
        debug("Operator: %c\n", input[*pos]);
        Token token = invalid_token;
        if (input[*pos] == '+') {
            token = add_token;
        } else if (input[*pos] == '-') {
            token = sub_token;
            (*pos)++;
            if (input[*pos] == '>') {
                token = convert_token;
            } else {
                (*pos)--;
            }
        } else if (input[*pos] == '*') {
            token = mul_token;
        } else if (input[*pos] == '/') {
            token = div_token;
        } else if (input[*pos] == '^'){
            token = caret_token;
        } else {
            token = equals_token;
        }
        (*pos)++;
        return token;
    }

    if (is_digit(input[*pos])) {
        debug("Number: %c\n", input[*pos]);
        double number = 0;
        while (is_digit(input[*pos])) {
            number = number * 10 + char_to_digit(input[*pos]);
            (*pos)++;
        }
        if (input[*pos] == '.') {
            (*pos)++;
            double decimal = 0.1;
            while (is_digit(input[*pos])) {
                number += char_to_digit(input[*pos]) * decimal;
                decimal /= 10;
                (*pos)++;
            }
        }
        if (input[*pos] == 'e' || input[*pos] == 'E') {
            (*pos)++;
            if (!is_digit(input[*pos])) {
                return invalid_token;
            }
            double power = 0;
            while (is_digit(input[*pos])) {
                power += power * 10 + char_to_digit(input[*pos]);
                (*pos)++;
            }
            // TODO: overflow check
            number *= pow(10, power);
        }
        return token_new_num(number);
    }

    return invalid_token;
}

typedef struct TokenString TokenString;
struct TokenString {
    Token *tokens;
    size_t length;
};

TokenString tokenize(const char *input, Arena *arena) {
    TokenString token_string;
    token_string.tokens = arena_alloc(arena, sizeof(Token) * MAX_INPUT);
    token_string.length = 0;
    bool done = false;
    size_t pos = 0;
    size_t input_length = strnlen(input, MAX_INPUT + 1);
    if (input_length > MAX_INPUT) {
        token_string.tokens[0] = invalid_token;
        token_string.length = 1;
        return token_string;
    }
    while (!done) {
        Token token = next_token(input, &pos, input_length, arena);
        if (token.type == TOK_INVALID || token.type == TOK_END || token.type == TOK_QUIT) {
            done = true;
        } else if (token.type == TOK_WHITESPACE) {
            continue;
        }
        token_string.tokens[token_string.length] = token;
        token_string.length++;
    }
    return token_string;
}

void token_display(Token token) {
    switch (token.type) {
        case TOK_END:
            debug("End of input\n");
            break;
        case TOK_INVALID:
            debug("Invalid token\n");
            break;
        case TOK_QUIT:
            debug("Quit token\n");
            break;
        case TOK_HELP:
            debug("Help token\n");
            break;
        case TOK_NUM:
            debug("Number: %f\n", token.number);
            break;
        case TOK_VAR:
            debug("Variable: %s\n", token.var_name);
            break;
        case TOK_EQUALS:
            debug("Equals token\n");
            break;
        case TOK_ADD:
            debug("Addition token\n");
            break;
        case TOK_SUB:
            debug("Subtraction token\n");
            break;
        case TOK_MUL:
            debug("Multiplication token\n");
            break;
        case TOK_DIV:
            debug("Division token\n");
            break;
        case TOK_WHITESPACE:
            debug("Whitespace token\n");
            break;
        case TOK_UNIT:
            debug("Unit token: %s\n", unit_strings[token.unit_type]);
            break;
        case TOK_CONVERT:
            debug("Convert token\n");
            break;
        case TOK_CARET:
            debug("Caret token\n");
            break;
        default:
            debug("Unknown token\n");
    }
}
