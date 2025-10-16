#pragma once

typedef union Vector3 {
    struct {float x, y, z;};
    float e[3];
} Vector3;

static inline Vector3 vector3_make(float x, float y, float z);
static inline Vector3 add_v3_v3   (Vector3 lhs, Vector3 rhs);
static inline Vector3 sub_v3_v3   (Vector3 lhs, Vector3 rhs);
static inline Vector3 mul_v3_float(Vector3 lhs, float   rhs);
static inline Vector3 div_v3_float(Vector3 lhs, float   rhs);

#ifdef GAME_MATH_IMPLEMENTATION
// operators for c++
#ifdef __cplusplus
static inline Vector3 operator+(Vector3 lhs, Vector3 rhs) {
    Vector3 v = {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
        .z = lhs.z + rhs.z,
    };
    return v;
}

static inline Vector3& operator+=(Vector3& lhs, Vector3& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;

    return lhs;
}

static inline Vector3 operator-(Vector3 lhs, Vector3 rhs) {
    Vector3 v = {
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
        .z = lhs.z - rhs.z,
    };
    return v;
}

static inline Vector3& operator-=(Vector3& lhs, Vector3& rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;

    return lhs;
}

static inline Vector3 operator*(Vector3 lhs, float rhs) {
    Vector3 v = {
        .x = lhs.x * rhs,
        .y = lhs.y * rhs,
        .z = lhs.z * rhs,
    };
    return v;
}

static inline Vector3& operator*=(Vector3& lhs, float rhs) {
    lhs.x *= rhs;
    lhs.y *= rhs;
    lhs.z *= rhs;

    return lhs;
}

static inline Vector3 operator/(Vector3 lhs, float rhs) {
    Vector3 v = {
        .x = lhs.x / rhs,
        .y = lhs.y / rhs,
        .z = lhs.z / rhs,
    };
    return v;
}

static inline Vector3& operator/=(Vector3& lhs, float rhs) {
    lhs.x /= rhs;
    lhs.y /= rhs;
    lhs.z /= rhs;

    return lhs;
}
#endif

static inline Vector3 vector3_make(float x, float y, float z) {
    Vector3 v = {
        .x = x,
        .y = y,
        .z = z
    };

    return v;
}

static inline Vector3 add_v3_v3   (Vector3 lhs, Vector3 rhs) {
    Vector3 v = {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
        .z = lhs.z + rhs.z,
    };

    return v;
}

static inline Vector3 sub_v3_v3   (Vector3 lhs, Vector3 rhs) {
    Vector3 v = {
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
        .z = lhs.z - rhs.z,
    };

    return v;
}

static inline Vector3 mul_v3_float(Vector3 lhs, float   rhs) {
    Vector3 v = {
        .x = lhs.x * rhs,
        .y = lhs.y * rhs,
        .z = lhs.z * rhs,
    };

    return v;
}

static inline Vector3 div_v3_float(Vector3 lhs, float   rhs) {
    Vector3 v = {
        .x = lhs.x / rhs,
        .y = lhs.y / rhs,
        .z = lhs.z / rhs,
    };

    return v;
}
#endif //GAME_MATH_IMPLEMENTATION