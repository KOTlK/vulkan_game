#pragma once

typedef union Vector2 {
    struct {
        float x, y;
    };
    float e[2];
} Vector2;

static inline Vector2 vector2_make(float x, float y);

static inline Vector2 clamp(Vector2 value, Vector2 min, Vector2 max);
static inline float   dot  (Vector2 lhs, Vector2 rhs);
static inline float   cross(Vector2 lhs, Vector2 rhs);

#if defined(GAME_MATH_IMPLEMENTATION)

static inline Vector2 vector2_make(float x, float y) {
    Vector2 v = {
        .x = x,
        .y = y
    };

    return v;
}

static inline float   dot  (Vector2 lhs, Vector2 rhs) {
    return lhs.x * rhs.x +
           lhs.y * rhs.y;
}

static inline float cross(Vector2 lhs, Vector2 rhs) {
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

static inline Vector2 clamp(Vector2 value, Vector2 min, Vector2 max) {
    return {
        .x = value.x < min.x ? min.x : (value.x > max.x ? max.x : value.x),
        .y = value.y < min.y ? min.y : (value.y > max.y ? max.y : value.y),
    };
}

#endif // GAME_MATH_IMPLEMENTATION