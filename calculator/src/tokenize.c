#pragma once

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "arena.c"

typedef enum Unit Unit;
enum Unit {
    // Distance
    UNIT_CENTIMETER,
    UNIT_METER,
    UNIT_KILOMETER,
    UNIT_INCH,
    UNIT_FOOT,
    UNIT_MILE,
    // Time
    UNIT_SECOND,
    UNIT_MINUTE,
    UNIT_HOUR,
    // Mass
    UNIT_GRAM,
    UNIT_KILOGRAM,
    UNIT_POUND,
    UNIT_OUNCE,

    UNIT_COUNT,
    UNIT_NONE,
    UNIT_UNKNOWN,
};

#define MAX_UNIT_STRING 3

const char *unit_strings[] = {
    // Distance
    "cm",
    "m",
    "km",
    "in",
    "ft",
    "mi",
    // Time
    "s",
    "min",
    "hr",
    // Mass
    "g",
    "kg",
    "lb",
    "oz",

    "",
    "none",
    "unknown",
};

typedef enum TokenType TokenType;
enum TokenType {
    TOK_NUM,
    TOK_UNIT,
    TOK_ADD,
    TOK_SUB,
    TOK_MUL,
    TOK_DIV,
    TOK_WHITESPACE,
    TOK_QUIT,
    TOK_END,
    TOK_INVALID,
};

typedef struct Token Token;
struct Token {
    TokenType type;
    Unit unit;
    double number;
};

#define MAX_INPUT 256

// TODO: turn this into something simpler at comptime
bool char_in_set(char c, const unsigned char *set) {
    bool char_set[256] = {0};
    for (size_t i = 0; set[i] != '\0'; i++) {
        char_set[set[i]] = true;
    }
    return char_set[c];
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
const Token add_token = {TOK_ADD};
const Token sub_token = {TOK_SUB};
const Token mul_token = {TOK_MUL};
const Token div_token = {TOK_DIV};

Token token_new_num(double num) {
    return (Token){TOK_NUM, .number = num };
}

Token token_new_unit(Unit unit) {
    return (Token){TOK_UNIT, .unit = unit};
}

// TODO: make this more generic where I can simply define
// basically a table of strings and their corresponding tokens
Token next_token(const char *input, size_t *pos, size_t length) {
    const unsigned char end[] = {'\0', '\n'};
    if (*pos >= length || char_in_set(input[*pos], end)) {
        if (*pos != length) return invalid_token;
        return end_token;
    }

    if (is_letter(input[*pos])) {
        char string_token[MAX_INPUT] = {0};
        for (size_t i = 0; is_letter(input[*pos]); i++) {
            string_token[i] = input[*pos];
            (*pos)++;
        }
        if (strnlen(string_token, 5) == 4
            && (strncmp(string_token, "quit", 4) == 0
                || strncmp(string_token, "exit", 4) == 0)) {
            return quit_token;
        }
        for (size_t unit = 0; unit < UNIT_COUNT; unit++) {
            const char *unit_str = unit_strings[unit];
            const size_t len = strnlen(unit_str, MAX_UNIT_STRING + 1);
            const size_t str_len = strnlen(string_token, MAX_UNIT_STRING + 1);
            if (str_len == len && strncmp(string_token, unit_str, len + 1) == 0) {
                return (Token){TOK_UNIT, .unit = unit};
            }
        }
        return invalid_token;
    }

    const unsigned char whitespace[] = {' ', '\t'};
    if (char_in_set(input[*pos], whitespace)) {
        while (char_in_set(input[*pos], whitespace)) {
            (*pos)++;
        }
        return whitespace_token;
    }

    const unsigned char operators[] = {'+', '-', '*', '/'};
    if (char_in_set(input[*pos], operators)) {
        Token token;
        if (input[*pos] == '+') {
            token = add_token;
        } else if (input[*pos] == '-') {
            token = sub_token;
        } else if (input[*pos] == '*') {
            token = mul_token;
        } else if (input[*pos] == '/') {
            token = div_token;
        }
        (*pos)++;
        return token;
    }

    if (is_digit(input[*pos])) {
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
        Token token = next_token(input, &pos, input_length);
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
            printf("End of input\n");
            break;
        case TOK_INVALID:
            printf("Invalid token\n");
            break;
        case TOK_QUIT:
            printf("Quit token\n");
            break;
        case TOK_NUM:
            printf("Number: %f\n", token.number);
            break;
        case TOK_ADD:
            printf("Addition token\n");
            break;
        case TOK_SUB:
            printf("Subtraction token\n");
            break;
        case TOK_MUL:
            printf("Multiplication token\n");
            break;
        case TOK_DIV:
            printf("Division token\n");
            break;
        default:
            printf("Unknown token\n");
            break;
    }
}
