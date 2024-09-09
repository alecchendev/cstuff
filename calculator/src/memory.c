#pragma once

#include "debug.c"
#include "expression.c"
#include "hash_map.c"

// Structures for tracking user defined things
// we want to track between different executions.

typedef struct Memory Memory;
struct Memory {
    HashMap vars; // string -> Expression
};

Memory memory_new(Arena *arena) {
    return (Memory) { .vars = hash_map_new(sizeof(Expression), arena) };
}

void memory_add_var(Memory *mem, unsigned char *var_name, Expression value, Arena *arena) {
    hash_map_insert(&mem->vars, (unsigned char *)var_name, (void *)&value, arena);
}

bool memory_contains_var(Memory mem, unsigned char *var_name) {
    debug("Checking for var: %s\n", var_name);
    for (size_t i = 0; i < mem.vars.capacity; i++) {
        if (mem.vars.exists[i]) {
            debug("Key: %s\n", mem.vars.items[i].key);
        }
    }
    bool result = hash_map_contains(mem.vars, (unsigned char *)var_name);
    debug("found: %d\n", result);
    return result;
}

const Expression memory_get_var(Memory mem, unsigned char *var_name) {
    assert(hash_map_contains(mem.vars, (unsigned char *)var_name));
    return *(Expression *)hash_map_get(mem.vars, (unsigned char *)var_name);
}

