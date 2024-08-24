
#include <unistd.h>
#include <stdio.h>
#include "arena.c"
#include "execute.c"
#include "parse.c"
#include "tokenize.c"

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
    if (a.type == TOK_WHITESPACE || a.type == TOK_INVALID || a.type == TOK_END || a.type == TOK_QUIT) {
        return true;
    }
    if (a.type == TOK_UNIT && a.unit_type != b.unit_type) {
        printf("Expected unit type %s, got %s\n", unit_strings[b.unit_type], unit_strings[a.unit_type]);
        return false;
    }
    if (a.type == TOK_NUM && a.number - b.number > 0.0000001) {
        printf("Expected number %f, got %f\n", b.number, a.number);
        return false;
    }
    return true;
}

void test_tokenize_case(void *c_opaque) {
    TokenCase c = *(TokenCase *)c_opaque;
    Arena arena = arena_create();
    TokenString tokens = tokenize(c.input, &arena);
    for (size_t i = 0; i < tokens.length; i++) {
        token_display(tokens.tokens[i]);
    }
    assert_eq(tokens.length, c.length);
    for (size_t i = 0; i < tokens.length; i++) {
        assert(tokens_equal(tokens.tokens[i], c.expected[i]));
    }
    arena_free(&arena);
}

void test_tokenize(void *_) {
    char max_len_input[MAX_INPUT + 1] = {0};
    memset(max_len_input, 'x', MAX_INPUT + 1);
    TokenCase cases[] = {
        {"", 1, {end_token}},
        {"1", 2, {token_new_num(1), end_token}},
        {"1 + 2", 4, {token_new_num(1), add_token, token_new_num(2), end_token}},
        {"\t 1\t+     2  ", 4, {token_new_num(1), add_token, token_new_num(2), end_token}},
        {"1 - 2 * 3 / 4", 8, {
            token_new_num(1), sub_token, token_new_num(2), mul_token,
            token_new_num(3), div_token, token_new_num(4), end_token
        }},
        {"45.874", 2, {token_new_num(45.874), end_token}},
        {"2e3", 2, {token_new_num(2000), end_token}},
        {"3.32e2", 2, {token_new_num(332), end_token}},
        {"3.32E2", 2, {token_new_num(332), end_token}},
        {"3.32e-2", 1, {invalid_token}}, // TODO: handle negatives generally + in scientific notation?
        {"asdf", 1, {invalid_token}},
        {"quit", 1, {quit_token}},
        {"exit", 1, {quit_token}},
        {"quitX", 1, {invalid_token}}, // TODO: this should be invalid
        {max_len_input, 1, {invalid_token}},
        // Units
        {"3 cm + 5.5min", 6, {
            token_new_num(3), token_new_unit(UNIT_CENTIMETER), add_token,
            token_new_num(5.5), token_new_unit(UNIT_MINUTE), end_token
        }},
        {"1 mi / h", 5, {
            token_new_num(1), token_new_unit(UNIT_MILE), div_token,
            token_new_unit(UNIT_HOUR), end_token
        }},
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
    const Expression *expected;
} ParseCase;

bool exprs_equal(Expression a, Expression b) {
    if (a.type != b.type) {
        printf("Expected type %d, got %d\n", b.type, a.type);
        return false;
    }
    if (a.type == EXPR_EMPTY || a.type == EXPR_QUIT) {
        return true;
    }
    if (a.type == EXPR_CONSTANT && a.expr.constant.value - b.expr.constant.value > 0.0000001) {
        printf("Expected number %f, got %f\n", b.expr.constant.value, a.expr.constant.value);
        return false;
    } else if (a.type == EXPR_CONSTANT) {
        return true;
    }
    assert(a.type == EXPR_ADD || a.type == EXPR_SUB || a.type == EXPR_MUL || a.type == EXPR_DIV);
    if (!exprs_equal(*a.expr.binary_expr.left, *b.expr.binary_expr.left)) {
        return false;
    }
    if (!exprs_equal(*a.expr.binary_expr.right, *b.expr.binary_expr.right)) {
        return false;
    }
    return true;
}

void test_parse_case(void *c_opaque) {
    ParseCase *c = (ParseCase *)c_opaque;
    Arena arena = arena_create();
    TokenString tokens = tokenize(c->input, &arena);
    for (size_t i = 0; i < tokens.length; i++) {
        token_display(tokens.tokens[i]);
    }
    Expression *expr = parse(tokens, &arena);
    assert(exprs_equal(*expr, *c->expected));
    arena_free(&arena);
}

void test_parse(void *_) {
    Arena case_arena = arena_create();
    UnitType mi_h[] = {UNIT_MILE, UNIT_HOUR};
    int mi_h_degrees[] = {1, -1};
    const ParseCase cases[] = {
        {"1 + 2 * 3", expr_new_bin(EXPR_ADD,
            expr_new_const(1, &case_arena),
            expr_new_bin(EXPR_MUL,
                expr_new_const(2, &case_arena),
                expr_new_const(3, &case_arena),
            &case_arena),
        &case_arena)},
        {"1 cm -2kg", expr_new_bin(EXPR_SUB,
            expr_new_const_unit(1, unit_new_single(UNIT_CENTIMETER, 1, &case_arena), &case_arena),
            expr_new_const_unit(2, unit_new_single(UNIT_KILOGRAM, 1, &case_arena), &case_arena),
        &case_arena)},
        {"5 mi / 4 h", expr_new_bin(EXPR_DIV,
            expr_new_const_unit(5, unit_new_single(UNIT_MILE, 1, &case_arena), &case_arena),
            expr_new_const_unit(4, unit_new_single(UNIT_HOUR, 1, &case_arena), &case_arena),
        &case_arena)},
        // TODO: invalid expression
    };
    const size_t num_cases = sizeof(cases) / sizeof(ParseCase);
    bool all_passed = true;
    for (size_t i = 0; i < num_cases; i++) {
        all_passed &= fork_test_case(i, test_parse_case, (void *)&cases[i],
                                     NULL, "Case %zu failed\n");
    }
    assert(all_passed);
    arena_free(&case_arena);
}

typedef struct {
    const char *input;
    const double expected;
} EvaluateCase;

void test_evaluate_case(void *c_opaque) {
    EvaluateCase *c = (EvaluateCase *)c_opaque;
    Arena arena = arena_create();
    TokenString tokens = tokenize(c->input, &arena);
    Expression *expr = parse(tokens, &arena);
    assert(expr != NULL);
    double result = evaluate(*expr);
    assert_eq(result, c->expected);
    arena_free(&arena);
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
        // Units
        {"2 cm * 3 + 1.5cm", 7.5},
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

typedef struct {
    const Unit unit;
    const char *expected;
} DisplayUnitCase;

void test_display_unit(void *_) {
    Arena arena = arena_create();
    DisplayUnitCase cases[] = {
        {unit_new_none(&arena), ""},
        {unit_new_single(UNIT_MINUTE, 1, &arena), "min"},
        {unit_new_single(UNIT_CENTIMETER, 2, &arena), "cm^2"},
        {unit_new_single(UNIT_KILOGRAM, -2, &arena), "kg^-2"},
        {unit_new_unknown(&arena), "unknown"},
    };
    const size_t num_cases = sizeof(cases) / sizeof(DisplayUnitCase);
    for (size_t i = 0; i < num_cases; i++) {
        char *displayed = display_unit(cases[i].unit, &arena);
        debug("Expected: %s got: %s\n", cases[i].expected, displayed);
        assert(strncmp(displayed, cases[i].expected, MAX_COMPOSITE_UNIT_STRING) == 0);
    }
    arena_free(&arena);
}

int main() {
    void (*tests[])(void *) = {
        test_tokenize,
        test_parse,
        test_evaluate,
        test_display_unit,
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
