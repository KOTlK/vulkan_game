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

#endif