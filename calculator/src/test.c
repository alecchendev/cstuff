
#include "parse.c"
#include "tokenize.c"
#include "arena.c"

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

bool test_tokenize() {
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
    bool results[num_cases];
    for (size_t i = 0; i < num_cases; i++) {
        results[i] = true;
        Arena *arena = arena_create(MAX_MEMORY_SIZE);
        TokenCase c = cases[i];
        TokenString tokens = tokenize(c.input, arena);
        if (tokens.length != c.length) {
            printf("Expected %zu tokens, got %zu\n", c.length, tokens.length);
            results[i] = false;
        } else {
            for (size_t j = 0; j < tokens.length; j++) {
                if (!tokens_equal(tokens.tokens[j], c.expected[j])) {
                    results[i] = false;
                    break;
                }
            }
        }
        arena_free(arena);
    }
    bool all_passed = true;
    for (size_t i = 0; i < num_cases; i++) {
        if (!results[i]) {
            printf("Case %zu failed\n", i);
            all_passed = false;
        }
    }
    return all_passed;
}

// TODO: allow running each test in its own process
// so we can safely handle crashes
int main() {
    bool results[] = {
        test_tokenize(),
    };
    for (int i = 0; i < sizeof(results) / sizeof(results[0]); i++) {
        if (!results[i]) {
            printf("Test %d failed\n", i);
            return 1;
        }
    }
    printf("All tests passed\n");
}
