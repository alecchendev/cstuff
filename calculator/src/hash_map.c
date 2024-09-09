#pragma once

#include <stdbool.h>
#include "arena.c"

// Basic hash map with string keys and generic values

#define HASH_MAP_INIT_CAPACITY 16
#define HASH_MAP_RESIZE_THRESHOLD 0.7

size_t djb2_hash(const unsigned char *key, size_t capacity) {
    size_t hash = 5381;
    for (size_t i = 0; key[i] != '\0'; i++){
        hash = ((hash << 5) + hash) + key[i];
    }
    return hash & (capacity - 1);
}

typedef struct KeyValue KeyValue;
struct KeyValue {
    const unsigned char *key;
    void *value;
};

typedef struct HashMap HashMap;
struct HashMap {
    size_t size;
    size_t capacity;
    size_t value_size;
    KeyValue *items;
    bool *exists;
};

void display_keys(HashMap map) {
    for (size_t i = 0; i < map.capacity; i++) {
        if (map.exists[i]) {
            debug("Key: %s\n", map.items[i].key);
        }
    }
}

bool is_pow_two(size_t num) {
    if (num == 0) return true;
    size_t bit_count = 0;
    for (size_t i = 0; i < sizeof(size_t) * 8; i++) {
        bit_count += (num & ((size_t)1 << i)) > 0;
    }
    return bit_count == 1;
}

HashMap hash_map_new_capacity(size_t capacity, size_t value_size, Arena *arena) {
    assert(is_pow_two(capacity));
    HashMap map = {
        .size = 0,
        .capacity = capacity,
        .value_size = value_size,
        .items = arena_alloc(arena, capacity * sizeof(KeyValue)),
        .exists = arena_alloc(arena, capacity * sizeof(bool)),
    };
    memset(map.items, 0, map.capacity);
    memset(map.exists, false, map.capacity);
    return map;
}

HashMap hash_map_new(size_t value_size, Arena *arena) {
    return hash_map_new_capacity(HASH_MAP_INIT_CAPACITY, value_size, arena);
}

void hash_map_insert_alloc(HashMap *map, const unsigned char *key, void *value, bool alloc, Arena *arena) {
    size_t init_idx = djb2_hash(key, map->capacity);
    size_t idx = init_idx;
    do {
        if (map->exists[idx] && strcmp((char *)map->items[idx].key, (char *)key) != 0) {
            idx = (idx + 1) & (map->capacity - 1);
        }
    } while (idx != init_idx);
    size_t key_len = strlen((char *)key) + 1;
    unsigned char *key_alloc = arena_alloc(arena, key_len);
    memcpy(key_alloc, key, key_len);
    map->items[idx] = (KeyValue) { .key = key_alloc, .value = value };
    if (alloc) {
        map->items[idx].value = arena_alloc(arena, map->value_size);
        memcpy(map->items[idx].value, value, map->value_size);
    }
    map->size += !map->exists[idx]; // Only increment if key not already there
    map->exists[idx] = true;
}

void hash_map_resize(HashMap *map, size_t new_capacity, Arena *arena) {
    assert(map->capacity < new_capacity);
    HashMap new_map = hash_map_new_capacity(new_capacity, map->value_size, arena);
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->exists[i]) {
            hash_map_insert_alloc(&new_map, map->items[i].key, map->items[i].value, false, arena);
        }
    }
    *map = new_map;
}

// `value` param MUST have the same size as the
// `value_size` this map was initialized with.
void hash_map_insert(HashMap *map, const unsigned char *key, void *value, Arena *arena) {
    if ((float)map->size / (float)map->capacity >= HASH_MAP_RESIZE_THRESHOLD) {
        hash_map_resize(map, map->capacity * 2, arena);
    }
    hash_map_insert_alloc(map, key, value, true, arena);
}

bool hash_map_contains(HashMap map, const unsigned char *key) {
    size_t init_idx = djb2_hash(key, map.capacity);
    size_t idx = init_idx;
    do {
        if (map.exists[idx] && strcmp((char *)map.items[idx].key, (char *)key) == 0) {
            return true;
        }
    } while (idx != init_idx);
    return false;
}

void *hash_map_get(HashMap map, const unsigned char *key) {
    size_t init_idx = djb2_hash(key, map.capacity);
    size_t idx = init_idx;
    do {
        if (map.exists[idx] && strcmp((char *)map.items[idx].key, (char *)key) == 0) {
            return map.items[idx].value;
        }
    } while (idx != init_idx);
    return NULL;
}
