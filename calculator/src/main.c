
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include "parse.c"
#include "tokenize.c"
#include "arena.c"

const size_t MAX_MEMORY_SIZE = MAX_INPUT * sizeof(Token) + MAX_INPUT * sizeof(Expression);

// TODO make this + repl more testable..?
bool execute_line(const char *input) {
    Arena *arena = arena_create(MAX_MEMORY_SIZE);
    TokenString tokens = tokenize(input, arena);
    Expression *expr = parse(tokens, arena);
    if (expr == NULL) {
        printf("Invalid expression\n");
    } else if (expr->type == EMPTY) {
        // no op
    } else if (expr->type == QUIT) {
        // is this a case?
        return true;
    } else {
        double result = evaluate(*expr);
        printf("%f\n", result);
    }
    arena_free(arena);
    return false;
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
    for (size_t i = input_len; i > input_pos; i--) {
        input[i + slice_len] = input[i];
    }
    for (size_t i = 0; i < slice_len; i++) {
        input[input_pos + i] = slice[i];
    }
}

void delete_slice(char input[MAX_INPUT], size_t start, size_t end) {
    if (end > MAX_INPUT + 1 || start >= end) return;
    size_t len = end - start;
    for (size_t i = start; i < MAX_INPUT - len; i++) {
        input[i] = input[i + len];
    }
}

void redraw_line(const char input[MAX_INPUT], size_t input_len, size_t input_pos) {
    printf("\x1B[2K\r%s%s", prompt, input);
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
#define MAX_HISTORY_MEMORY (MAX_HISTORY * sizeof(Input))

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

    Arena *history_arena = arena_create(MAX_HISTORY_MEMORY);
    History history = { .history = NULL, .len = 0, .pos = 0 };
    history.history = arena_alloc(history_arena, sizeof(Input) * MAX_HISTORY);

    bool done = false;
    while (!done) {
        // Cache for latest input if we scroll through history
        Input latest_input = { .data = {0}, .len = 0 };
        Input input = { .data = {0}, .len = 0 };
        size_t input_pos = 0;
        printf("%s", prompt);

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
        done = execute_line(input.data);

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
    arena_free(history_arena);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        repl(stdin);
        return 0;
    } else if (argc == 2) {
        const char *input = argv[1];
        execute_line(input);
    } else {
        printf("Usage: %s [input in quotes]\n", argv[0]);
    }
    return 0;
}
