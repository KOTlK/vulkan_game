#pragma once

#include <math.h>
#include "Vector3.h"
#include "assert.h"
#include "mathematics.h"

union Quaternion {
    struct { float x, y, z, w; };
    float e[4];
};

static inline Quaternion quaternion_identity = {{0, 0, 0, 1}};

inline Quaternion  operator+(const Quaternion& a, const Quaternion& b);
inline Quaternion& operator+=(Quaternion& a, const Quaternion& b);
inline Quaternion  operator-(const Quaternion& a, const Quaternion& b);
inline Quaternion& operator-=(Quaternion& a, const Quaternion& b);
inline Quaternion& operator-(Quaternion& q);
inline Quaternion  operator-(const Quaternion& q);
inline Quaternion  operator*(const Quaternion& a, const Quaternion& b);
inline Vector3     operator*(const Quaternion& a, const Vector3& b);
inline Quaternion  operator*(const Quaternion& a, const float b);

static inline Quaternion quaternion_make(const float x, const float y, const float z, const float w);
static inline Quaternion quaternion_angle_axis(const float angle, const Vector3& axis);
static inline Quaternion quaternion_angle_axis(const float angle, const float axis[3]);
static inline void       quaternion_to_angle_axis(const Quaternion& q, float& angle, Vector3& axis);
static inline Quaternion quaternion_euler(const Vector3& euler);
static inline Quaternion conjugate(const Quaternion& q);
static inline Quaternion inverse(const Quaternion& q);
static inline float      magnitude(const Quaternion& q);
static inline void       normalize(Quaternion& q);
static inline Quaternion normalized(const Quaternion& q);
static inline float      dot(const Quaternion& a, const Quaternion& b);
static inline Quaternion lerp(const Quaternion& a, const Quaternion& b, float t);
static inline Quaternion nlerp(const Quaternion& a, const Quaternion& b, float t);
static inline Quaternion slerp(const Quaternion& a, const Quaternion& b, const float t);

#ifdef GAME_MATH_IMPLEMENTATION

#include "Vector3.h"

inline Quaternion  operator+(const Quaternion& a, const Quaternion& b) {
    Quaternion res = {{
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w
    }};

    return res;
}

inline Quaternion& operator+=(Quaternion& a, const Quaternion& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;

    return a;
}

inline Quaternion  operator-(const Quaternion& a, const Quaternion& b) {
    Quaternion res = {{
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w
    }};

    return res;
}

inline Quaternion& operator-=(Quaternion& a, const Quaternion& b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    a.w -= b.w;

    return a;
}

inline Quaternion& operator-(Quaternion& q) {
    q.x = -q.x;
    q.y = -q.y;
    q.z = -q.z;
    q.w = -q.w;

    return q;
}

inline Quaternion operator-(const Quaternion& q) {
    Quaternion res = {
        .x = -q.x,
        .y = -q.y,
        .z = -q.z,
        .w = -q.w
    };

    return res;
}

inline Quaternion  operator*(const Quaternion& a, const Quaternion& b) {
    Quaternion res = {{
        a.w*b.x + b.w*a.x + a.y*b.z - b.y*a.z,
        a.w*b.y + b.w*a.y + a.z*b.x - b.z*a.x,
        a.w*b.z + b.w*a.z + a.x*b.y - b.x*a.y,
        a.w*b.w - b.w*a.x - a.y*b.y - a.z*b.z
    }};

    normalize(res);

    return res;
}

inline Vector3 operator*(const Quaternion& q, const Vector3& v) {
    Vector3 qv = vector3_make(q.x, q.y, q.z);
    Vector3 c1 = cross(qv, v) * 2.0f;
    Vector3 c2 = cross(qv, c1);

    return v + c1 * q.w + c2;
}

inline Quaternion operator*(const Quaternion& a, const float b) {
    Quaternion res = {
        .x = a.x*b,
        .y = a.y*b,
        .z = a.z*b,
        .w = a.w*b
    };

    normalize(res);

    return res;
}

static inline Quaternion quaternion_make(const float x, const float y, const float z, const float w) {
    Quaternion q = {
        .x = x,
        .y = y,
        .z = z,
        .w = w
    };

    normalize(q);

    return q;
}

static inline Quaternion quaternion_angle_axis(const float angle, const Vector3& axis) {
    Quaternion res{};
    float      half = angle * 0.5f;
    float      hs   = sinf(half);
    float      cs   = cosf(half);
    float      mag  = magnitude(axis);
    Vector3    v    = axis;

    if (fabs(mag) > 1.0f) {
        v = normalized(v);
    }

    res.w = cs;
    res.x = v.x * hs;
    res.y = v.y * hs;
    res.z = v.z * hs;

    normalize(res);

    return res;
}

static inline Quaternion quaternion_angle_axis(const float angle, const float axis[3]) {
    Quaternion res{};
    float      half = angle * 0.5f;
    float      hs   = sinf(half);
    float      cs   = cosf(half);
    Vector3    v    = vector3_make(axis[0], axis[1], axis[2]);
    float      mag  = magnitude(v);

    if (fabs(mag) > 1.0f) {
        v = normalized(v);
    }

    res.w = cs;
    res.x = v.x * hs;
    res.y = v.y * hs;
    res.z = v.z * hs;

    normalize(res);

    return res;
}

static inline Quaternion quaternion_euler(const Vector3& euler) {
    float cy = cosf(euler.z * 0.5f);
    float sy = sinf(euler.z * 0.5f);
    float cp = cosf(euler.y * 0.5f);
    float sp = sinf(euler.y * 0.5f);
    float cr = cosf(euler.x * 0.5f);
    float sr = sinf(euler.x * 0.5f);

    Quaternion q;
    q.w =  cr * cp * cy + sr * sp * sy;
    q.x =  sr * cp * cy - cr * sp * sy;
    q.y =  cr * sp * cy + sr * cp * sy;
    q.z =  cr * cp * sy - sr * sp * cy;

    normalize(q);
    
    return q;
}

static inline void quaternion_to_angle_axis(const Quaternion& q, float& angle, Vector3& axis) {
    float w = q.w;
    float s = sqrtf(1.0f - w * w);

    angle = 2.0f * acosf(w);

    if (s < 1e-6f) {
        axis = {{1, 0, 0}};
        angle = 0.0f;
        return;
    }

    axis.x = q.x / s;
    axis.y = q.y / s;
    axis.z = q.z / s;
}

static inline Quaternion conjugate(const Quaternion& q) {
    Quaternion res = {
        .x = -q.x,
        .y = -q.y,
        .z = -q.z,
        .w = q.w
    };

    return res;
}

static inline Quaternion inverse(const Quaternion& q) {
    Quaternion res = conjugate(q);

    float norm = q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z;

    res.x = res.x / norm;
    res.y = res.y / norm;
    res.z = res.z / norm;
    res.w = res.w / norm;

    return res;
}

static inline float magnitude(const Quaternion& q) {
    return sqrtf(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
}

static inline void normalize(Quaternion& q) {
    float mag = magnitude(q);
    q.x = q.x / mag;
    q.y = q.y / mag;
    q.z = q.z / mag;
    q.w = q.w / mag;
}

static inline Quaternion normalized(const Quaternion& q) {
    Quaternion res {};
    float mag = magnitude(q);
    res.x = q.x / mag;
    res.y = q.y / mag;
    res.z = q.z / mag;
    res.w = q.w / mag;

    return res;
}

static inline float dot(const Quaternion& a, const Quaternion& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

static inline Quaternion lerp(const Quaternion& a, const Quaternion& b, float t) {
    Quaternion res = {{
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        a.z + t * (b.z - a.z),
        a.w + t * (b.w - a.w),
    }};

    return res;
}

static inline Quaternion nlerp(const Quaternion& a, const Quaternion& b, float t) {
    float d = dot(a, b);

    Quaternion bb = (d < 0.0f) ? -b : b;

    Quaternion result = {{
        a.x + t * (bb.x - a.x),
        a.y + t * (bb.y - a.y),
        a.z + t * (bb.z - a.z),
        a.w + t * (bb.w - a.w),
    }};

    return normalized(result);
}

static inline Quaternion slerp(const Quaternion& a, const Quaternion& b, const float t) {
    Assert(fabs(1.0f - magnitude(a)) <= FLOAT_EPSILON, "Quaternion a should be normalized.");
    Assert(fabs(1.0f - magnitude(b)) <= FLOAT_EPSILON, "Quaternion b should be normalized.");
    float      d  = dot(a, b);
    Quaternion bb = b;

    if (d < 0.0f) {
        d  = -d;
        bb = -bb;
    }

    float angle = acosf(d);

    // fallback to nlerp if angle is too low
    if (angle <= 0.001f) {
        return nlerp(a, b, t);
    }

    float s     = sinf(angle);

    float wa = sinf((1.0f - t) * angle) / s;
    float wb = sinf(t * angle) / s;

    return a * wa + bb * wb;
}

#endif