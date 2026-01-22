module;

#include <math.h>
#include "debug.h"

export module vector3;

export union Vector3 {
    struct {float x, y, z;};
    float e[3];
};

export constexpr Vector3 vector3_right   = {{ 1.0f,  0.0f,  0.0f  }};
export constexpr Vector3 vector3_up      = {{ 0.0f,  1.0f,  0.0f  }};
export constexpr Vector3 vector3_forward = {{ 0.0f,  0.0f,  1.0f  }};

export constexpr Vector3 vector3_left    = {{ -1.0f, 0.0f,  0.0f  }};
export constexpr Vector3 vector3_down    = {{ 0.0f,  -1.0f, 0.0f  }};
export constexpr Vector3 vector3_back    = {{ 0.0f,  0.0f,  -1.0f }};


export inline Vector3 vector3_make(float x, float y, float z) {
    Vector3 v = {
        .x = x,
        .y = y,
        .z = z
    };

    return v;
}

export inline Vector3 operator+(const Vector3 lhs, const Vector3 rhs) {
    Vector3 v = {
        .x = lhs.x + rhs.x,
        .y = lhs.y + rhs.y,
        .z = lhs.z + rhs.z,
    };
    return v;
}

export inline Vector3& operator+=(Vector3& lhs, const Vector3& rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;

    return lhs;
}

export inline Vector3 operator-(const Vector3 lhs, const Vector3 rhs) {
    Vector3 v = {
        .x = lhs.x - rhs.x,
        .y = lhs.y - rhs.y,
        .z = lhs.z - rhs.z,
    };
    return v;
}

export inline Vector3  operator-(const Vector3& v) {
    return vector3_make(-v.x, -v.y, -v.z);
}

export inline void  operator-(Vector3& v) {
    v.x = -v.x;
    v.y = -v.y;
    v.z = -v.z;
}

export inline Vector3& operator-=(Vector3& lhs, const Vector3& rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;

    return lhs;
}

export inline Vector3 operator*(const Vector3 lhs, const float rhs) {
    Vector3 v = {
        .x = lhs.x * rhs,
        .y = lhs.y * rhs,
        .z = lhs.z * rhs,
    };
    return v;
}

export inline Vector3& operator*=(Vector3& lhs, const float rhs) {
    lhs.x *= rhs;
    lhs.y *= rhs;
    lhs.z *= rhs;

    return lhs;
}

export inline Vector3 operator/(const Vector3 lhs, const float rhs) {
    Vector3 v = {
        .x = lhs.x / rhs,
        .y = lhs.y / rhs,
        .z = lhs.z / rhs,
    };
    return v;
}

export inline Vector3& operator/=(Vector3& lhs, const float rhs) {
    lhs.x /= rhs;
    lhs.y /= rhs;
    lhs.z /= rhs;

    return lhs;
}

export inline Vector3 cross(const Vector3& lhs, const Vector3& rhs) {
    Vector3 v = {
        .x = lhs.y * rhs.z - lhs.z * rhs.y,
        .y = lhs.z * rhs.x - lhs.x * rhs.z,
        .z = lhs.x * rhs.y - lhs.y * rhs.x
    };

    return v;
}

export inline Vector3 clamp(const Vector3& value, const Vector3& min, const Vector3& max) {
    return {
        .x = value.x < min.x ? min.x : (value.x > max.x ? max.x : value.x),
        .y = value.y < min.y ? min.y : (value.y > max.y ? max.y : value.y),
        .z = value.z < min.z ? min.z : (value.z > max.z ? max.z : value.z),
    };
}

export inline float dot(const Vector3& lhs, const Vector3& rhs) {
    return lhs.x * rhs.x + 
           lhs.y * rhs.y +
           lhs.z * rhs.z;
}

export inline float   magnitude(const Vector3& v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}

export inline float   sqr_magnitude(const Vector3& v) {
    return v.x*v.x + v.y*v.y + v.z*v.z;
}

export inline void    normalize(Vector3& v) {
    float mag = magnitude(v);

    v.x /= mag;
    v.y /= mag;
    v.z /= mag;
}

export inline Vector3 normalized(const Vector3& v) {
    float mag = magnitude(v);
    Vector3 res = {
        .x = v.x / mag,
        .y = v.y / mag,
        .z = v.z / mag,
    };

    return res;
}

export inline void    vector3_print(Vector3& v) {
    Logf("%f, %f, %f", v.x, v.y, v.z);
}