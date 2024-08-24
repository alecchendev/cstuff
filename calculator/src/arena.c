#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "log.c"

// A basic arena allocator that doubles in size when it runs out of memory.
typedef struct Arena Arena;
struct Arena {
    size_t size;
    size_t used;
    unsigned char *memory;
};

#define DEFAULT_ARENA_SIZE 1024

Arena arena_create() {
    Arena arena;
    arena.size = DEFAULT_ARENA_SIZE;
    arena.used = 0;
    arena.memory = malloc(arena.size);
    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    while (arena->used + size > arena->size) {
        debug("Used: %zu, Size: %zu, Requested: %zu -> doubling arena size.\n", arena->used, arena->size, size);
        arena->size *= 2;
        unsigned char *new_memory = malloc(arena->size);
        memcpy(new_memory, arena->memory, arena->used);
        free(arena->memory);
        arena->memory = new_memory;
    }
    if (arena->memory == NULL) {
        printf("Failed to allocate memory\n");
        return NULL;
    }
    void *ptr = arena->memory + arena->used;
    arena->used += size;
    return ptr;
}

void arena_free(Arena *arena) {
    free(arena->memory);
    arena->size = 0;
    arena->used = 0;
}

