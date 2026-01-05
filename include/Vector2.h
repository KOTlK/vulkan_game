#pragma once

typedef union Vector2 {
    struct {
        float x, y;
    };
    float e[2];
} Vector2;

static inline Vector2 vector2_make(float x, float y);

#if defined(GAME_MATH_IMPLEMENTATION)

static inline Vector2 vector2_make(float x, float y) {
    Vector2 v = {
        .x = x,
        .y = y
    };

    return v;
}

#endif // GAME_MATH_IMPLEMENTATION