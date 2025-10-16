#pragma once

#include "Vector3.h"
#include "Vector2.h"
#include "Matrix4.h"

static inline float   dot  (Vector2 lhs, Vector2 rhs);
static inline float   dot  (Vector3 lhs, Vector3 rhs);
static inline float   cross(Vector2 lhs, Vector2 rhs);
static inline Vector3 cross(Vector3 lhs, Vector3 rhs);


#ifdef GAME_MATH_IMPLEMENTATION
static inline float   dot  (Vector2 lhs, Vector2 rhs) {
    return lhs.x * rhs.x +
           lhs.y * rhs.y;
}

static inline float   dot  (Vector3 lhs, Vector3 rhs) {
    return lhs.x * rhs.x + 
           lhs.y * rhs.y +
           lhs.z * rhs.z;
}

static inline float cross(Vector2 lhs, Vector2 rhs) {
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

static inline Vector3 cross(Vector3 lhs, Vector3 rhs) {
    Vector3 v = {
        .x = lhs.y * rhs.z - lhs.z * rhs.y,
        .y = lhs.z * rhs.x - lhs.x * rhs.z,
        .z = lhs.x * rhs.y - lhs.y * rhs.x
    };

    return v;
}
#endif // GAME_MATH_IMPLEMENTATION