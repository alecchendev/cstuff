#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "arena.c"

typedef enum {
    NOOP,
    ADD,
    SUB,
    MUL,
    DIV
} BinaryOp;

enum TokenType {
    NUMBER,
    BINARY_OPERATOR,
    WHITESPACE,
    QUITTOKEN, // TODO: how to name QUIT without conflicts
    END,
    INVALID,
};

typedef union TokenData TokenData;
union TokenData {
    double number;
    BinaryOp binary_operator;
};

typedef struct Token Token;
struct Token {
    enum TokenType type;
    union TokenData data;
};

const unsigned int MAX_INPUT = 256;
const char *prompt = ">>> ";

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

const Token invalid_token = {INVALID};
const Token end_token = {END};
const Token quit_token = {QUITTOKEN};
const Token whitespace_token = {WHITESPACE};

Token token_new_bin(BinaryOp op) {
    return (Token){BINARY_OPERATOR, .data = { .binary_operator = op }};
}

Token token_new_num(double num) {
    return (Token){NUMBER, .data = { .number = num }};
}

// TODO: make this more generic where I can simply define
// basically a table of strings and their corresponding tokens
Token next_token(const char *input, size_t *pos, size_t length) {
    const unsigned char end[] = {'\0', '\n'};
    if (*pos >= length || char_in_set(input[*pos], end)) {
        if (*pos != length) return invalid_token;
        return end_token;
    }

    if (input[*pos] == 'q' || input[*pos] == 'e') {
        // TODO: handle no spaces after?
        if (strncmp(input + *pos, "quit", 4) == 0 || strncmp(input + *pos, "exit", 4) == 0) {
            *pos += 4;
            return quit_token;
        }
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
        BinaryOp op;
        switch (input[*pos]) {
            case '+':
                op = ADD;
                break;
            case '-':
                op = SUB;
                break;
            case '*':
                op = MUL;
                break;
            case '/':
                op = DIV;
                break;
        }
        (*pos)++;
        return token_new_bin(op);
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
            while (input[*pos] >= '0' && input[*pos] <= '9') {
                number += (input[*pos] - '0') * decimal;
                decimal /= 10;
                (*pos)++;
            }
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
    size_t input_length = strnlen(input, MAX_INPUT);
    while (!done) {
        Token token = next_token(input, &pos, input_length);
        if (token.type == INVALID || token.type == END || token.type == QUITTOKEN) {
            done = true;
        } else if (token.type == WHITESPACE) {
            continue;
        }
        token_string.tokens[token_string.length] = token;
        token_string.length++;
    }
    return token_string;
}

void token_display(Token token) {
    switch (token.type) {
        case END:
            printf("End of input\n");
            break;
        case INVALID:
            printf("Invalid token\n");
            break;
        case QUITTOKEN:
            printf("Quit token\n");
            break;
        case NUMBER:
            printf("Number: %f\n", token.data.number);
            break;
        case BINARY_OPERATOR:
            printf("Operator: %d\n", token.data.binary_operator);
            break;
        default:
            printf("Unknown token\n");
            break;
    }
}
