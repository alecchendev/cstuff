#pragma once

#include "hash_map.c"
#include "expression.c"

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
    // TODO: debug display map and what var we're requesting
    return hash_map_contains(&mem.vars, (unsigned char *)var_name);
}

Expression memory_get_var(Memory mem, unsigned char *var_name) {
    assert(hash_map_contains(&mem.vars, (unsigned char *)var_name));
    return *(Expression *)hash_map_get(&mem.vars, (unsigned char *)var_name);
}

