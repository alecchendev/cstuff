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

typedef enum UnitCategory UnitCategory;
enum UnitCategory {
    UNIT_CATEGORY_DISTANCE,
    UNIT_CATEGORY_TIME,
    UNIT_CATEGORY_MASS,
    UNIT_CATEGORY_NONE,
};

UnitCategory unit_category(UnitType type) {
    switch (type) {
        case UNIT_CENTIMETER:
        case UNIT_METER:
        case UNIT_KILOMETER:
        case UNIT_INCH:
        case UNIT_FOOT:
        case UNIT_MILE:
            return UNIT_CATEGORY_DISTANCE;
        case UNIT_SECOND:
        case UNIT_MINUTE:
        case UNIT_HOUR:
            return UNIT_CATEGORY_TIME;
        case UNIT_GRAM:
        case UNIT_KILOGRAM:
        case UNIT_POUND:
        case UNIT_OUNCE:
            return UNIT_CATEGORY_MASS;
        case UNIT_COUNT:
        case UNIT_NONE:
        case UNIT_UNKNOWN:
            return UNIT_CATEGORY_NONE;
    }
}

double unit_convert(UnitType a, UnitType b);

double unit_convert_through(UnitType a, UnitType b, UnitType c) {
    return unit_convert(a, c) * unit_convert(c, b);
}

// TODO: just convert this into a simple table
double unit_convert(UnitType a, UnitType b) {
    if (a == b) return 1;
    // Everything is defined big -> small, otherwise reuse other conversions
    switch (a) {
        case UNIT_CENTIMETER:
            switch (b) {
                case UNIT_METER: return 1 / unit_convert(b, a);
                case UNIT_KILOMETER: return 1 / unit_convert(b, a);
                case UNIT_INCH: return 1 / unit_convert(b, a);
                case UNIT_FOOT: return 1 / unit_convert(b, a);
                case UNIT_MILE: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_METER:
            switch (b) {
                case UNIT_CENTIMETER: return 100;
                case UNIT_KILOMETER: return 1 / unit_convert(b, a);
                case UNIT_INCH: return unit_convert_through(a, b, UNIT_FOOT);
                case UNIT_FOOT: return 3.2808399;
                case UNIT_MILE: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_KILOMETER:
            switch (b) {
                case UNIT_CENTIMETER: return unit_convert_through(a, b, UNIT_METER);
                case UNIT_METER: return 1000;
                case UNIT_INCH: return unit_convert_through(a, b, UNIT_FOOT);
                case UNIT_FOOT: return unit_convert_through(a, b, UNIT_METER);
                case UNIT_MILE: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_INCH:
            switch (b) {
                case UNIT_CENTIMETER: return 2.54;
                case UNIT_METER: return 1 / unit_convert(b, a);
                case UNIT_KILOMETER: return 1 / unit_convert(b, a);
                case UNIT_FOOT: return 1 / unit_convert(b, a);
                case UNIT_MILE: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_FOOT:
            switch (b) {
                case UNIT_CENTIMETER: return unit_convert_through(a, b, UNIT_INCH);
                case UNIT_METER: return 1 / unit_convert(b, a);
                case UNIT_KILOMETER: return 1 / unit_convert(b, a);
                case UNIT_INCH: return 12;
                case UNIT_MILE: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_MILE:
            switch (b) {
                case UNIT_CENTIMETER: return unit_convert_through(a, b, UNIT_METER);
                case UNIT_METER: return unit_convert_through(a, b, UNIT_KILOMETER);
                case UNIT_KILOMETER: return 1.609344;
                case UNIT_INCH: return unit_convert_through(a, b, UNIT_FOOT);
                case UNIT_FOOT: return 5280;
                default: return 0;
            }
        case UNIT_SECOND:
            switch (b) {
                case UNIT_MINUTE: return 1 / unit_convert(b, a);
                case UNIT_HOUR: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_MINUTE:
            switch (b) {
                case UNIT_SECOND: return 60;
                case UNIT_HOUR: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_HOUR:
            switch (b) {
                case UNIT_SECOND: return unit_convert_through(a, b, UNIT_MINUTE);
                case UNIT_MINUTE: return 60;
                default: return 0;
            }
        case UNIT_GRAM:
            switch (b) {
                case UNIT_KILOGRAM: return 1 / unit_convert(b, a);
                case UNIT_OUNCE: return 1 / unit_convert(b, a);
                case UNIT_POUND: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_KILOGRAM:
            switch (b) {
                case UNIT_GRAM: return 1000;
                case UNIT_OUNCE: return unit_convert_through(a, b, UNIT_POUND);
                case UNIT_POUND: return 2.20462262;
                default: return 0;
            }
        case UNIT_OUNCE:
            switch (b) {
                case UNIT_GRAM: return 28.3495231;
                case UNIT_KILOGRAM: return 1 / unit_convert(b, a);
                case UNIT_POUND: return 1 / unit_convert(b, a);
                default: return 0;
            }
        case UNIT_POUND:
            switch (b) {
                case UNIT_GRAM: return unit_convert_through(a, b, UNIT_OUNCE);
                case UNIT_KILOGRAM: return 1 / unit_convert(b, a);
                case UNIT_OUNCE: return 16;
                default: return 0;
            }
        case UNIT_COUNT:
        case UNIT_NONE:
        case UNIT_UNKNOWN:
            return 0;
    }
}

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

Unit unit_combine(Unit a, Unit b, Arena *arena) {
    bool *a_leftover = arena_alloc(arena, a.length * sizeof(bool));
    bool *b_leftover = arena_alloc(arena, b.length * sizeof(bool));
    memset(a_leftover, true, a.length * sizeof(bool));
    memset(b_leftover, true, b.length * sizeof(bool));
    size_t length = a.length + b.length;
    for (size_t i = 0; i < b.length; i++) {
        for (size_t j = 0; j < a.length; j++) {
            if (a.types[j] == b.types[i] && a.degrees[j] == -b.degrees[i]) {
                a_leftover[j] = false;
                b_leftover[i] = false;
                length -= 2;
            } else if (a.types[j] == b.types[i]) {
                a.degrees[j] += b.degrees[i];
                b_leftover[i] = false;
                length -= 1;
            }
        }
    }
    UnitType *types = arena_alloc(arena, length * sizeof(UnitType));
    int *degrees = arena_alloc(arena, length * sizeof(int));
    size_t curr_length = 0;
    for (size_t i = 0; i < a.length; i++) {
        if (a_leftover[i]) {
            types[curr_length] = a.types[i];
            degrees[curr_length] = a.degrees[i];
            curr_length++;
        }
    }
    for (size_t i = 0; i < b.length; i++) {
        if (b_leftover[i]) {
            types[curr_length] = b.types[i];
            degrees[curr_length] = b.degrees[i];
            curr_length++;
        }
    }
    return (Unit) { .types = types, .degrees = degrees, .length = length };
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
        return "none";
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
