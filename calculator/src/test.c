
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

bool fork_test_case(size_t case_num, void (*test)(void *), void *c_opaque,
                    const char *pass_str, const char *fail_str) {
    pid_t pid = fork();
    assert(pid != -1);
    if (pid == 0) {
        test(c_opaque);
        exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            if (pass_str != NULL) printf(pass_str, case_num);
            return true;
        } else if (WIFSIGNALED(status)) {
            if (fail_str != NULL) printf(fail_str, case_num);
            return false;
        } else {
            printf("Child process exited abnormally\n");
            return false;
        }
    }
}

const size_t MAX_MEMORY_SIZE = MAX_INPUT * sizeof(Token) + MAX_INPUT * sizeof(Expression);

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
            if (a.data.number - b.data.number > 0.0000001) {
                printf("Expected number %f, got %f\n", b.data.number, a.data.number);
                return false;
            }
            return true;
        case BINARY_OPERATOR:
            if (a.data.binary_operator != b.data.binary_operator) {
                printf("Expected operator %d, got %d\n", b.data.binary_operator, a.data.binary_operator);
                return false;
            }
            return true;
        default:
            return false;
    }
}

void test_tokenize_case(void *c_opaque) {
    TokenCase c = *(TokenCase *)c_opaque;
    Arena *arena = arena_create(MAX_MEMORY_SIZE);
    TokenString tokens = tokenize(c.input, arena);
    assert_eq(tokens.length, c.length);
    for (size_t i = 0; i < tokens.length; i++) {
        assert(tokens_equal(tokens.tokens[i], c.expected[i]));
    }
    arena_free(arena);
}

void test_tokenize(void *_) {
    char max_len_input[MAX_INPUT + 1] = {0};
    memset(max_len_input, 'x', MAX_INPUT + 1);
    TokenCase cases[] = {
        {"", 1, {end_token}},
        {"1", 2, {token_new_num(1), end_token}},
        {"1 + 2", 4, {token_new_num(1), token_new_bin(ADD), token_new_num(2), end_token}},
        {"\t 1\t+     2  ", 4, {token_new_num(1), token_new_bin(ADD), token_new_num(2), end_token}},
        {"1 - 2 * 3 / 4", 8, {
            token_new_num(1), token_new_bin(SUB), token_new_num(2), token_new_bin(MUL),
            token_new_num(3), token_new_bin(DIV), token_new_num(4), end_token
        }},
        {"45.874", 2, {token_new_num(45.874), end_token}},
        {"asdf", 1, {invalid_token}},
        {"quit", 1, {quit_token}},
        {"exit", 1, {quit_token}},
        {"quitX", 1, {invalid_token}}, // TODO: this should be invalid
        {max_len_input, 1, {invalid_token}},
        // TODO: more comprehensive
    };
    const size_t num_cases = sizeof(cases) / sizeof(TokenCase);
    bool all_passed = true;
    for (size_t i = 0; i < num_cases; i++) {
        all_passed &= fork_test_case(i, test_tokenize_case, (void *)&cases[i],
                                     NULL, "Case %zu failed\n");
    }
    assert(all_passed);
}

typedef struct {
    const char *input;
    const double expected;
} EvaluateCase;

void test_evaluate_case(void *c_opaque) {
    EvaluateCase *c = (EvaluateCase *)c_opaque;
    Arena *arena = arena_create(MAX_MEMORY_SIZE);
    TokenString tokens = tokenize(c->input, arena);
    Expression *expr = parse(tokens, arena);
    assert(expr != NULL);
    double result = evaluate(*expr);
    assert_eq(result, c->expected);
    arena_free(arena);
}

void test_evaluate(void *_) {
    const EvaluateCase cases[] = {
        // Basic ops
        {"42", 42},
        {"9 + 10", 19},
        {"20 - 30", -10},
        {"21 * 2", 42},
        {"1 / 2", 0.5},
        // Order of operations
        {"1 + 2 * 3", 7},
        {"2 / 4 + 1", 1.5},
        // Everything
        {" 3000  - 600 * 20 / 2.5 +\t20", -1780},
        // TODO: overflow tests
    };
    const size_t num_cases = sizeof(cases) / sizeof(EvaluateCase);
    bool all_passed = true;
    for (size_t i = 0; i < num_cases; i++) {
        all_passed &= fork_test_case(i, test_evaluate_case, (void *)&cases[i],
                                     NULL, "Case %zu failed\n");
    }
    assert(all_passed);
}

int main() {
    void (*tests[])(void *) = {
        test_tokenize,
        test_evaluate,
    };
    const size_t n_tests = sizeof(tests) / sizeof(tests[0]);
    bool all_passed = true;
    for (size_t i = 0; i < n_tests; i++) {
        all_passed &= fork_test_case(i, tests[i], NULL, "Test %zu passed\n", "Test %zu failed\n");
    }
    if (all_passed) {
        printf("All tests passed\n");
    } else {
        printf("Some tests failed\n");
    }
}
