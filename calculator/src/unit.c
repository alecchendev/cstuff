#pragma once

#include <stdbool.h>
#include <string.h>
#include "arena.c"
#include "log.c"

typedef enum UnitType UnitType;
enum UnitType {
    // Distance
    UNIT_CENTIMETER,
    UNIT_METER,
    UNIT_KILOMETER,
    UNIT_INCH,
    UNIT_FOOT,
    UNIT_MILE,
    // Time
    UNIT_SECOND,
    UNIT_MINUTE,
    UNIT_HOUR,
    // Mass
    UNIT_GRAM,
    UNIT_KILOGRAM,
    UNIT_POUND,
    UNIT_OUNCE,

    UNIT_COUNT,
    UNIT_NONE,
    UNIT_UNKNOWN,
};

#define MAX_UNIT_STRING 3

const char *unit_strings[] = {
    // Distance
    "cm",
    "m",
    "km",
    "in",
    "ft",
    "mi",
    // Time
    "s",
    "min",
    "h",
    // Mass
    "g",
    "kg",
    "lb",
    "oz",

    "",
    "none",
    "unknown",
};

typedef struct Unit Unit;
struct Unit {
    UnitType *types;
    int *degrees;
    size_t length;
};

bool is_unit_none(Unit unit) {
    return unit.length == 1 && unit.types[0] == UNIT_NONE;
}

bool is_unit_unknown(Unit unit) {
    return unit.length == 1 && unit.types[0] == UNIT_UNKNOWN;
}

Unit unit_new(UnitType types[], int degrees[], size_t length, Arena *arena) {
    UnitType *new_types = arena_alloc(arena, length * sizeof(UnitType));
    int *new_degrees = arena_alloc(arena, length * sizeof(int));
    memcpy(new_types, types, length * sizeof(UnitType));
    memcpy(new_degrees, degrees, length * sizeof(int));
    return (Unit) { .types = new_types, .degrees = new_degrees, .length = length };
}

Unit unit_new_single(UnitType type, int degree, Arena *arena) {
    return unit_new(&type, &degree, 1, arena);
}

Unit unit_new_none(Arena *arena) {
    return unit_new_single(UNIT_NONE, 0, arena);
}

Unit unit_new_unknown(Arena *arena) {
    return unit_new_single(UNIT_UNKNOWN, 0, arena);
}

bool units_equal(Unit a, Unit b) {
    if (a.length != b.length) return false;
    for (size_t i = 0; i < a.length; i++) {
        if (a.types[i] != b.types[i]) return false;
        if (a.degrees[i] != b.degrees[i]) return false;
    }
    return true;
}

#define MAX_UNITS_DISPLAY 32
#define MAX_DEGREE_STRING 4
#define MAX_UNIT_WITH_DEGREE_STRING MAX_UNIT_STRING + MAX_DEGREE_STRING
#define MAX_COMPOSITE_UNIT_STRING MAX_UNITS_DISPLAY * MAX_UNIT_WITH_DEGREE_STRING

char *display_unit(Unit unit, Arena *arena) {
    if (unit.length == 0 || (unit.length == 1 && unit.types[0] == UNIT_NONE)) {
        return "";
    }
    if (unit.length == 1 && unit.types[0] == UNIT_UNKNOWN) {
        return "unknown";
    }
    char *str = arena_alloc(arena, MAX_COMPOSITE_UNIT_STRING);
    strncpy(str, "", 1);
    for (size_t i = 0; i < unit.length; i++) {
        char unit_str[MAX_UNIT_WITH_DEGREE_STRING];
        if (unit.degrees[i] == 1) {
            sprintf(unit_str, "%s", unit_strings[unit.types[i]]);
        } else {
            sprintf(unit_str, "%s^%d", unit_strings[unit.types[i]], unit.degrees[i]);
        }
        strncat(str, unit_str, MAX_UNIT_WITH_DEGREE_STRING);
        if (i < unit.length - 1) {
            strncat(str, " ", 1);
        }
    }
    return str;
}
