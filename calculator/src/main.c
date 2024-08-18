
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

    bool done = false;
    while (!done) {
        char input[MAX_INPUT] = {0};
        printf("%s", prompt);

        size_t pos = 0;
        while (true) {
            UserInput key = read_user_input(input_fd);
            if (key.type == ENTER) {
                printf("\n");
                break;
            } else if (key.type == PRINTABLE && pos < MAX_INPUT) {
                printf("%c", key.c);
                input[pos] = key.c;
                pos++;
            } else if (key.type == BACKSPACE && pos > 0) {
                // TODO: handle backspace in the middle of input*/
                // \033 = escape, [1D moves left, [K clears to end of line*/
                printf("\033[1D\033[K");
                pos--;
                input[pos] = 0;
            // TODO: handle arrow keys
            } else if (key.type == UP) {
                printf("UP");
            } else if (key.type == DOWN) {
                printf("DOWN");
            } else if (key.type == LEFT) {
                printf("LEFT");
            } else if (key.type == RIGHT) {
                printf("RIGHT");
            }
        }
        done = execute_line(input);
    }
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
