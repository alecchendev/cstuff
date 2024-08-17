
#include <unistd.h>
#include <stdio.h>
#include "parse.c"
#include "tokenize.c"
#include "arena.c"

#define assert(condition) \
    ((condition) ? (void)0 : \
    __assert_fail(#condition, __FUNCTION__, __FILE__, __LINE__))

void __assert_fail(const char *condition, const char *function, const char *file, 
                   unsigned int line) {
    fprintf(stderr, "Assertion failed: %s, function %s, file %s, line %u.\n",
            condition, function, file, line);
    abort();
}

// TODO: do some sort of generic display to show
// the expected and actual values
#define assert_eq(a, b) assert((a) == (b))

const size_t MAX_MEMORY_SIZE = MAX_INPUT * sizeof(Token);

typedef struct TokenCase TokenCase;
struct TokenCase {
    const char *input;
    const size_t length;
    const Token expected[MAX_INPUT];
};

bool tokens_equal(Token a, Token b) {
    if (a.type != b.type) {
        printf("Expected type %d, got %d\n", b.type, a.type);
        return false;
    }
    switch (a.type) {
        case END:
        case INVALID:
        case QUITTOKEN:
        case NUMBER:
            return a.data.number == b.data.number;
        case BINARY_OPERATOR:
            return a.data.binary_operator == b.data.binary_operator;
        default:
            return false;
    }
}

void test_tokenize_case(TokenCase c) {
    Arena *arena = arena_create(MAX_MEMORY_SIZE);
    TokenString tokens = tokenize(c.input, arena);
    assert_eq(tokens.length, c.length);
    for (size_t i = 0; i < tokens.length; i++) {
        assert(tokens_equal(tokens.tokens[i], c.expected[i]));
    }
    arena_free(arena);
}

void test_tokenize() {
    TokenCase cases[] = {
        {"", 1, {end_token}},
        {"1", 2, {token_new_num(1), end_token}},
        {"1 + 2", 4, {token_new_num(1), token_new_bin(ADD), token_new_num(2), end_token}},
        {"1 - 2 * 3 / 4", 8, {
            token_new_num(1), token_new_bin(SUB), token_new_num(2), token_new_bin(MUL),
            token_new_num(3), token_new_bin(DIV), token_new_num(4), end_token
        }},
        {"asdf", 1, {invalid_token}},
        {"quit", 1, {quit_token}},
        {"exit", 1, {quit_token}},
        {"quitX", 1, {quit_token}},
        // TODO: more comprehensive
    };
    const size_t num_cases = sizeof(cases) / sizeof(TokenCase);
    bool all_passed = true;
    for (size_t i = 0; i < num_cases; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            printf("Fork failed for case %zu", i);
            assert(false);
        } else if (pid == 0) {
            test_tokenize_case(cases[i]);
            return;
        } else {
            int status;
            waitpid(pid, &status, 0);
            if (WIFSIGNALED(status)) {
                printf("Case %zu failed\n", i);
                all_passed = false;
            } else if (!WIFEXITED(status)) {
                printf("Child process exited abnormally: %d\n", status);
            }
        }
    }
    assert(all_passed);
}

int main() {
    void (*tests[])(void) = {
        test_tokenize,
    };
    const size_t n_tests = sizeof(tests) / sizeof(tests[0]);
    bool all_passed = true;
    for (size_t i = 0; i < n_tests; i++) {
        // Spin off a separate process for each test
        // so we can handle crashes and continue running
        // the rest of the tests
        pid_t pid = fork();
        if (pid == -1) {
            printf("Fork failed for test %zu", i);
            exit(1);
        } else if (pid == 0) {
            // Child -> run test
            tests[i]();
            return 0;
        } else {
            // Parent -> check test
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                printf("Test %zu passed\n", i);
            } else if (WIFSIGNALED(status)) {
                printf("Test %zu failed\n", i);
                all_passed = false;
            } else {
                printf("Child process exited abnormally\n");
            }
        }
    }
    if (all_passed) {
        printf("All tests passed\n");
    } else {
        printf("Some tests failed\n");
    }
}
