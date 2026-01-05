#pragma once

#include "types.h"

#ifdef min
#undef min 
#endif

#ifdef max
#undef max 
#endif

template <typename T>
static inline T clamp(T value, T min, T max);

template <typename T>
static inline T min(T lhs, T rhs);

template <typename T>
static inline T max(T lhs, T rhs);

static inline u32 next_power_of_2(u32 v);

#ifdef GAME_MATH_IMPLEMENTATION

template <typename T>
static inline T clamp(T value, T min, T max) {
    return value < min ? min : (value > max ? max : value);
}

template <typename T>
static inline T min(T lhs, T rhs) {
    return lhs < rhs ? lhs : rhs;
}

template <typename T>
static inline T max(T lhs, T rhs) {
    return lhs > rhs ? lhs : rhs;
}

static inline u32 next_power_of_2(u32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

#endif