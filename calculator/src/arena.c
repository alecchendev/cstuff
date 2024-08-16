#pragma once

#include <stdlib.h>
#include <stdio.h>

// The most braindead arena allocator possible.
typedef struct Arena Arena;
struct Arena {
    size_t size;
    size_t used;
    char memory[];
};

Arena *arena_create(size_t size) {
    Arena *arena = malloc(sizeof(Arena) + size);
    arena->size = size;
    arena->used = 0;
    // arena->memory already points to the start of the memory block
    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    if (arena->used + size > arena->size) {
        printf("ERROR: Out of memory\n");
        return NULL; // TODO
    }
    void *ptr = arena->memory + arena->used;
    arena->used += size;
    return ptr;
}

void arena_free(Arena *arena) {
    free(arena);
}

