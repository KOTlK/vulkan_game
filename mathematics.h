#pragma once

#include "types.h"

static inline u8 clamp(u8 value, u8 min, u8 max) {
    return value < min ? min : (value > max ? max : value);
}

static inline s8 clamp(s8 value, s8 min, s8 max) {
    return value < min ? min : (value > max ? max : value);
}

static inline u16 clamp(u16 value, u16 min, u16 max) {
    return value < min ? min : (value > max ? max : value);
}

static inline s16 clamp(s16 value, s16 min, s16 max) {
    return value < min ? min : (value > max ? max : value);
}

static inline u32 clamp(u32 value, u32 min, u32 max) {
    return value < min ? min : (value > max ? max : value);
}

static inline s32 clamp(s32 value, s32 min, s32 max) {
    return value < min ? min : (value > max ? max : value);
}

static inline u64 clamp(u64 value, u64 min, u64 max) {
    return value < min ? min : (value > max ? max : value);
}

static inline s64 clamp(s64 value, s64 min, s64 max) {
    return value < min ? min : (value > max ? max : value);
}

static inline float clamp(float value, float min, float max) {
    return value < min ? min : (value > max ? max : value);
}

static inline double clamp(double value, double min, double max) {
    return value < min ? min : (value > max ? max : value);
}