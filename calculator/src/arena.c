#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debug.c"

typedef struct ArenaBlock ArenaBlock;
struct ArenaBlock {
    size_t size;
    size_t used;
    ArenaBlock *next;
    unsigned char memory[];
};

// A basic arena allocator that doubles in size when it runs out of memory.
typedef struct Arena Arena;
struct Arena {
    ArenaBlock *first;
};

#define DEFAULT_ARENA_SIZE 1024

ArenaBlock *arena_block_create(size_t size) {
    ArenaBlock *block = malloc(sizeof(ArenaBlock) + size);
    assert(block != NULL);
    block->size = size;
    block->used = 0;
    block->next = NULL;
    return block;
}

Arena arena_create() {
    Arena arena;
    arena.first = arena_block_create(DEFAULT_ARENA_SIZE);
    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    ArenaBlock *block = arena->first;
    ArenaBlock *prev = arena->first;
    while (block != NULL) {
        if (block->used + size <= block->size) {
            void *ptr = block->memory + block->used;
            block->used += size;
            return ptr;
        }
        prev = block;
        block = block->next;
    }
    assert(prev != NULL);
    size_t new_size = prev->size * 2 >= size ? prev->size * 2 : size;
    debug("Allocating new block, curr_size: %zu new_size: %zu requested: %zu\n", prev->size, new_size, size);
    ArenaBlock *new_block = arena_block_create(new_size);
    new_block->used += size;
    prev->next = new_block;
    return (void *)new_block->memory;
}

void arena_free(Arena *arena) {
    ArenaBlock *next;
    for (ArenaBlock *block = arena->first; block != NULL; block = next) {
        next = block->next;
        free(block);
    }
}

void arena_clear(Arena *arena) {
    ArenaBlock *next;
    for (ArenaBlock *block = arena->first; block != NULL; block = next) {
        next = block->next;
        block->size = 0;
    }
}
