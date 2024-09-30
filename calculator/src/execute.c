
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "arena.c"
#include "evaluate.c"
#include "expression.c"
#include "memory.c"
#include "parse.c"
#include "tokenize.c"

// TODO: more math, everything relating to a stored memory
const char help_msg[] = "Hello! Here's some stuff you can do:\n\
Math: 1 + 2 * 3 - 4 / 5\n\
More math: TODO pow, log, parentheses\n\
Math with units: 1km/2s*3km+4km^2s^-1\n\
Convert units: 10 m/s^2 -> km/h^2\n\
Auto-convert units: 10 km - 2 m + 12 mi\n\
Variables: x = 9 + 10\n\
Custom units: 1 pentameter -> 5 m\n\
Unit aliases: n = kg m s^-2";

bool execute_line(const char *input, char *output, size_t output_len, Memory *mem, Arena *repl_arena) {
    Arena arena = arena_create();
    TokenString tokens = tokenize(input, &arena);
    Expression expr = parse(tokens, *mem, &arena);
    substitute_variables(&expr, *mem);
    display_expr(0, expr, &arena);
    bool quit = false;
    ErrorString err = err_empty();
    if (!check_valid_expr(expr, &err, &arena)) {
        memcpy(output, err.msg, err.len);
    } else if (expr.type == EXPR_EMPTY) {
        memset(output, 0, output_len);
    } else if (expr.type == EXPR_HELP) {
        memcpy(output, help_msg, sizeof(help_msg));
    } else if (expr.type == EXPR_QUIT) {
        memset(output, 0, output_len);
        quit = true;
    } else if (expr.type == EXPR_SET_VAR) {
        memset(output, 0, output_len);
        unsigned char * var_name = expr.expr.binary_expr.left->expr.var_name;
        Expression value = *expr.expr.binary_expr.right;
        Unit unit = check_unit(value, *mem, &err, &arena);
        if (is_unit_unknown(unit)) {
            memcpy(output, err.msg, err.len);
        } else {
            if (expr_is_number(value.type)) {
                double result = evaluate(value, *mem, &err, &arena);
                if (err.len > 0) {
                    memcpy(output, err.msg, err.len);
                } else {
                    value = expr_new_const_unit(result, expr_new_unit_full(unit, repl_arena),
                        repl_arena);
                    snprintf(output, output_len, "%s = %g %s", var_name, result, display_unit(unit, &arena));
                }
            } else {
                value = expr_new_unit_full(unit, repl_arena);
                snprintf(output, output_len, "%s = %s", var_name, display_unit(unit, &arena));
            }
            if (err.len == 0) {
                memory_add_var(mem, var_name, value, repl_arena);
            }
        }
    } else {
        Unit unit = check_unit(expr, *mem, &err, &arena);
        if (is_unit_unknown(unit)) {
            memcpy(output, err.msg, err.len);
        } else if (expr_is_number(expr.type)) {
            double result = evaluate(expr, *mem, &err, &arena);
            if (err.len > 0) {
                memcpy(output, err.msg, err.len);
            } else {
                snprintf(output, output_len, "%g %s", result, display_unit(unit, &arena));
            }
        } else {
            snprintf(output, output_len, "%s", display_unit(unit, &arena));
        }
    }
    arena_free(&arena);
    return quit;
}

typedef enum UserInputType UserInputType;
enum UserInputType {
    PRINTABLE,
    BACKSPACE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    ENTER,
    UNHANDLED,
};

typedef struct UserInput UserInput;
struct UserInput {
    UserInputType type;
    // Will be 0 for anything except PRINTABLE
    char c;
};

UserInput read_user_input(FILE *input_fd) {
    UserInput input = { .type = UNHANDLED, .c = 0 };
    char c = fgetc(input_fd);
    if (c == 27) {
        // Escape -> arrow keys
        c = fgetc(input_fd);
        if (c == '[') {
            c = fgetc(input_fd);
            if (c == 'A') {
                input.type = UP;
            } else if (c == 'B') {
                input.type = DOWN;
            } else if (c == 'C') {
                input.type = RIGHT;
            } else if (c == 'D') {
                input.type = LEFT;
            }
        }
    } else if (c == 127 || c == 8) {
        // Delete, backspace
        input.type = BACKSPACE;
    } else if (c == '\n') {
        input.type = ENTER;
    } else if (isprint(c)) {
        input.type = PRINTABLE;
        input.c = c;
    }
    return input;
}

void insert_slice(char input[MAX_INPUT], size_t input_len, size_t input_pos, char slice[MAX_INPUT], size_t slice_len) {
    if (input_pos > MAX_INPUT || input_len + slice_len > MAX_INPUT) return;
    char temp[MAX_INPUT] = {0};
    memcpy(temp, &input[input_pos], input_len - input_pos);
    memcpy(&input[input_pos], slice, slice_len);
    memcpy(&input[input_pos + slice_len], temp, input_len - input_pos);
}

void delete_slice(char input[MAX_INPUT], size_t start, size_t end) {
    if (end > MAX_INPUT + 1 || start >= end) return;
    size_t len = end - start;
    for (size_t i = start; i < MAX_INPUT - len; i++) {
        input[i] = input[i + len];
    }
}

#define PROMPT ">>> "

void redraw_line(const char input[MAX_INPUT], size_t input_len, size_t input_pos) {
    printf("\x1B[2K\r%s%s", PROMPT, input);
    if (input_len > input_pos) {
        printf("\x1B[%zuD", input_len - input_pos);
    }
}

typedef struct Input Input;
struct Input {
    char data[MAX_INPUT];
    size_t len;
};

typedef struct History History;
struct History {
    Input *history;
    size_t len;
    size_t pos;
};

#define MAX_HISTORY 64

void repl(FILE *input_fd) {
    const int file_num = fileno(input_fd);
    struct termios termios_start;
    if (tcgetattr(file_num, &termios_start) == -1) {
        printf("tcgetattr failed");
        exit(1); // TODO handle differently?
    }
    struct termios termios_new;
    termios_new = termios_start;
    // ECHO so we can intercept keys before they're printed
    // ICANON so we can process keys as they come in not after a newline
    termios_new.c_lflag &= ~(ECHO | ICANON);
    // VMIN = min number of bytes to read before processing
    // VTIME = max time to wait for input (0 = no timeout)
    termios_new.c_cc[VMIN] = 1;
    termios_new.c_cc[VTIME] = 0;
    if (tcsetattr(file_num, TCSANOW, &termios_new) == -1) {
        printf("tcsetattr failed");
        exit(1); // TODO handle differently?
    }

    Arena repl_arena = arena_create();
    History history = { .history = NULL, .len = 0, .pos = 0 };
    history.history = arena_alloc(&repl_arena, sizeof(Input) * MAX_HISTORY);

    Memory memory = memory_new(&repl_arena);

    bool done = false;
    while (!done) {
        // Cache for latest input if we scroll through history
        Input latest_input = { .data = {0}, .len = 0 };
        Input input = { .data = {0}, .len = 0 };
        size_t input_pos = 0;
        printf("%s", PROMPT);

        while (true) {
            UserInput key = read_user_input(input_fd);
            if (key.type == ENTER) {
                printf("\n");
                break;
            } else if (key.type == PRINTABLE && input.len < MAX_INPUT) {
                insert_slice(input.data, input.len, input_pos, &key.c, 1);
                input_pos++;
                input.len++;
                redraw_line(input.data, input.len, input_pos);
            } else if (key.type == BACKSPACE && input_pos > 0) {
                input_pos--;
                input.len--;
                delete_slice(input.data, input_pos, input_pos + 1);
                redraw_line(input.data, input.len, input_pos);
            } else if (key.type == UP && history.pos > 0) {
                if (history.pos == history.len) {
                    latest_input = input;
                }
                history.pos--;
                input = history.history[history.pos];
                input_pos = input.len;
                redraw_line(input.data, input.len, input_pos);
            } else if (key.type == DOWN && history.pos < history.len) {
                history.pos++;
                if (history.pos == history.len) {
                    input = latest_input;
                } else {
                    input = history.history[history.pos];
                }
                input_pos = input.len;
                redraw_line(input.data, input.len, input_pos);
            } else if (key.type == LEFT && input_pos > 0) {
                input_pos--;
                printf("\033[1D");
            } else if (key.type == RIGHT && input_pos < input.len) {
                input_pos++;
                printf("\033[1C");
            }
        }
        if (input.len == 0) {
            continue;
        }
        char output[512] = {0};
        done = execute_line(input.data, output, sizeof(output), &memory, &repl_arena);
        if (strnlen(output, sizeof(output)) > 0) printf("%s\n", output);

        if (history.len > 0 && strncmp(input.data, history.history[history.len - 1].data, MAX_INPUT) == 0) {
            continue;
        }

        // MAX_HISTORY is small enough that we can just shift everything
        if (history.len == MAX_HISTORY) {
            for (size_t i = 0; i < MAX_HISTORY - 1; i++) {
                history.history[i] = history.history[i + 1];
            }
            history.history[MAX_HISTORY - 1] = input;
        } else {
            history.history[history.len] = input;
            history.len++;
        }
        history.pos = history.len;
    }
    arena_free(&repl_arena);
}

