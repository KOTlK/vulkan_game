#pragma once

#include <math.h>
#include "Vector3.h"
#include "quaternion.h"

typedef union Matrix4 {
    // column-major, right-handed
    struct {
        float m0, m4, m8,  m12;
        float m1, m5, m9,  m13;
        float m2, m6, m10, m14;
        float m3, m7, m11, m15;
    };
    float e[16];
} Matrix4;

static inline Matrix4 matrix4_identity = {{1, 0, 0, 0,
                                          0, 1, 0, 0,
                                          0, 0, 1, 0,
                                          0, 0, 0, 1}};

static inline Matrix4 matrix4_make(float m0, float m4, float m8,  float m12,
                                   float m1, float m5, float m9,  float m13,
                                   float m2, float m6, float m10, float m14,
                                   float m3, float m7, float m11, float m15);

static inline Matrix4 matrix4_add(const Matrix4& lhs, const Matrix4& rhs);
static inline Matrix4 matrix4_sub(const Matrix4& lhs, const Matrix4& rhs);
static inline Matrix4 matrix4_mul(const Matrix4& lhs, const Matrix4& rhs);
static inline float   matrix4_det(const Matrix4& mat);
static inline void    matrix4_transpose(Matrix4& mat);
static inline Matrix4 matrix4_transposed(const Matrix4& mat);
static inline Matrix4 matrix4_transform(const float x, const float y, const float z);
static inline Matrix4 matrix4_transform(const Vector3 v);
// static inline Matrix4 matrix4_transform_2d(const float x, const float y);
// static inline Matrix4 matrix4_rotate_2d(float angle);
// static inline Matrix4 matrix4_scale_2d(float x, float y);
static inline Matrix4 matrix4_scale(const float x, const float y, const float z);
static inline Matrix4 matrix4_scale(const Vector3 v);
// static inline Matrix4 matrix4_trs_2d(float px, float py, float angle, float sx, float sy);
// static inline Matrix4 matrix4_camera_view_2d(Vector3 position);
// static inline Matrix4 matrix4_ortho_2d(float left, float right, float top, float bottom);
static inline Matrix4 matrix4_rotate(const Quaternion& q);
static inline Matrix4 matrix4_trs(const Vector3& p, const Quaternion& r, const Vector3& s);
static inline Matrix4 matrix4_perspective(float fov, 
                                          float aspect, 
                                          float near_plane, 
                                          float far_plane);

static inline Matrix4 matrix4_view(const Vector3& p, const Quaternion& r);

static inline Matrix4 matrix4_vp(const Vector3& eye_pos, 
                                 const Quaternion& r,
                                 float fov,
                                 float aspect,
                                 float near_plane,
                                 float far_plane);

static inline Matrix4 matrix4_mvp(const Vector3& p, 
                                  const Quaternion& r, 
                                  const Vector3& s,
                                  const Vector3& eye_p,
                                  const Quaternion& eye_r,
                                  float fov,
                                  float aspect,
                                  float near_plane,
                                  float far_plane);

static inline void matrix4_print(Matrix4& m);

static inline Matrix4  operator+(const Matrix4& lhs, const Matrix4& rhs);
static inline Matrix4& operator+=(Matrix4& lhs, const Matrix4& rhs);
static inline Matrix4  operator-(const Matrix4& lhs, const Matrix4& rhs);
static inline Matrix4& operator-=(Matrix4& lhs, const Matrix4& rhs);
static inline Matrix4  operator*(const Matrix4& lhs, const Matrix4& rhs);

// #define GAME_MATH_IMPLEMENTATION 
#ifdef GAME_MATH_IMPLEMENTATION

#include "mathematics.h"
#include "debug.h"

static inline Matrix4 matrix4_make(float m0, float m4, float m8,  float m12,
                                   float m1, float m5, float m9,  float m13,
                                   float m2, float m6, float m10, float m14,
                                   float m3, float m7, float m11, float m15) {
    Matrix4 matrix = {
        .m0  = m0,
        .m4  = m4,
        .m8  = m8,
        .m12 = m12,
        .m1  = m1,
        .m5  = m5,
        .m9  = m9,
        .m13 = m13,
        .m2  = m2,
        .m6  = m6,
        .m10 = m10,
        .m14 = m14,
        .m3  = m3,
        .m7  = m7,
        .m11 = m11,
        .m15 = m15,
    };

    return matrix;
}

static inline Matrix4  operator+(const Matrix4& lhs, const Matrix4& rhs) {
    Matrix4 res {
        .m0 = lhs.m0 + rhs.m0,
        .m4 = lhs.m4 + rhs.m4,
        .m8 = lhs.m8 + rhs.m8,
        .m12 = lhs.m12 + rhs.m12,
        .m1 = lhs.m1 + rhs.m1,
        .m5 = lhs.m5 + rhs.m5,
        .m9 = lhs.m9 + rhs.m9,
        .m13 = lhs.m13 + rhs.m13,
        .m2 = lhs.m2 + rhs.m2,
        .m6 = lhs.m6 + rhs.m6,
        .m10 = lhs.m10 + rhs.m10,
        .m14 = lhs.m14 + rhs.m14,
        .m3 = lhs.m3 + rhs.m3,
        .m7 = lhs.m7 + rhs.m7,
        .m11 = lhs.m11 + rhs.m11,
        .m15 = lhs.m15 + rhs.m15
    };

    return res;
}

static inline Matrix4& operator+=(Matrix4& lhs, const Matrix4& rhs) {
    lhs.m0 += rhs.m0;
    lhs.m4 += rhs.m4;
    lhs.m8 += rhs.m8;
    lhs.m12 += rhs.m12;
    lhs.m1 += rhs.m1;
    lhs.m5 += rhs.m5;
    lhs.m9 += rhs.m9;
    lhs.m13 += rhs.m13;
    lhs.m2 += rhs.m2;
    lhs.m6 += rhs.m6;
    lhs.m10 += rhs.m10;
    lhs.m14 += rhs.m14;
    lhs.m3 += rhs.m3;
    lhs.m7 += rhs.m7;
    lhs.m11 += rhs.m11;
    lhs.m15 += rhs.m15;

    return lhs;
}

static inline Matrix4  operator-(const Matrix4& lhs, const Matrix4& rhs) {
    Matrix4 res {
        .m0 = lhs.m0 - rhs.m0,
        .m4 = lhs.m4 - rhs.m4,
        .m8 = lhs.m8 - rhs.m8,
        .m12 = lhs.m12 - rhs.m12,
        .m1 = lhs.m1 - rhs.m1,
        .m5 = lhs.m5 - rhs.m5,
        .m9 = lhs.m9 - rhs.m9,
        .m13 = lhs.m13 - rhs.m13,
        .m2 = lhs.m2 - rhs.m2,
        .m6 = lhs.m6 - rhs.m6,
        .m10 = lhs.m10 - rhs.m10,
        .m14 = lhs.m14 - rhs.m14,
        .m3 = lhs.m3 - rhs.m3,
        .m7 = lhs.m7 - rhs.m7,
        .m11 = lhs.m11 - rhs.m11,
        .m15 = lhs.m15 - rhs.m15
    };

    return res;
}

static inline Matrix4& operator-=(Matrix4& lhs, const Matrix4& rhs) {
    lhs.m0 -= rhs.m0;
    lhs.m4 -= rhs.m4;
    lhs.m8 -= rhs.m8;
    lhs.m12 -= rhs.m12;
    lhs.m1 -= rhs.m1;
    lhs.m5 -= rhs.m5;
    lhs.m9 -= rhs.m9;
    lhs.m13 -= rhs.m13;
    lhs.m2 -= rhs.m2;
    lhs.m6 -= rhs.m6;
    lhs.m10 -= rhs.m10;
    lhs.m14 -= rhs.m14;
    lhs.m3 -= rhs.m3;
    lhs.m7 -= rhs.m7;
    lhs.m11 -= rhs.m11;
    lhs.m15 -= rhs.m15;
    
    return lhs;
}

static inline Matrix4  operator*(const Matrix4& lhs, const Matrix4& rhs) {
    Matrix4 res {
        .m0  = lhs.m0*rhs.m0  + lhs.m1*rhs.m4  + lhs.m2*rhs.m8   + lhs.m3*rhs.m12,
        .m1  = lhs.m0*rhs.m1  + lhs.m1*rhs.m5  + lhs.m2*rhs.m9   + lhs.m3*rhs.m13,
        .m2  = lhs.m0*rhs.m2  + lhs.m1*rhs.m6  + lhs.m2*rhs.m10  + lhs.m3*rhs.m14,
        .m3  = lhs.m0*rhs.m3  + lhs.m1*rhs.m7  + lhs.m2*rhs.m11  + lhs.m3*rhs.m15,
        .m4  = lhs.m4*rhs.m0  + lhs.m5*rhs.m4  + lhs.m6*rhs.m8   + lhs.m7*rhs.m12,
        .m5  = lhs.m4*rhs.m1  + lhs.m5*rhs.m5  + lhs.m6*rhs.m9   + lhs.m7*rhs.m13,
        .m6  = lhs.m4*rhs.m2  + lhs.m5*rhs.m6  + lhs.m6*rhs.m10  + lhs.m7*rhs.m14,
        .m7  = lhs.m4*rhs.m3  + lhs.m5*rhs.m7  + lhs.m6*rhs.m11  + lhs.m7*rhs.m15,
        .m8  = lhs.m8*rhs.m0  + lhs.m9*rhs.m4  + lhs.m10*rhs.m8  + lhs.m11*rhs.m12,
        .m9  = lhs.m8*rhs.m1  + lhs.m9*rhs.m5  + lhs.m10*rhs.m9  + lhs.m11*rhs.m13,
        .m10 = lhs.m8*rhs.m2  + lhs.m9*rhs.m6  + lhs.m10*rhs.m10 + lhs.m11*rhs.m14,
        .m11 = lhs.m8*rhs.m3  + lhs.m9*rhs.m7  + lhs.m10*rhs.m11 + lhs.m11*rhs.m15,
        .m12 = lhs.m12*rhs.m0 + lhs.m13*rhs.m4 + lhs.m14*rhs.m8  + lhs.m15*rhs.m12,
        .m13 = lhs.m12*rhs.m1 + lhs.m13*rhs.m5 + lhs.m14*rhs.m9  + lhs.m15*rhs.m13,
        .m14 = lhs.m12*rhs.m2 + lhs.m13*rhs.m6 + lhs.m14*rhs.m10 + lhs.m15*rhs.m14,
        .m15 = lhs.m12*rhs.m3 + lhs.m13*rhs.m7 + lhs.m14*rhs.m11 + lhs.m15*rhs.m15,
    };

    return res;
}

static inline Matrix4 matrix4_add(const Matrix4& lhs, const Matrix4& rhs) {
    Matrix4 res {
        .m0 = lhs.m0 + rhs.m0,
        .m4 = lhs.m4 + rhs.m4,
        .m8 = lhs.m8 + rhs.m8,
        .m12 = lhs.m12 + rhs.m12,
        .m1 = lhs.m1 + rhs.m1,
        .m5 = lhs.m5 + rhs.m5,
        .m9 = lhs.m9 + rhs.m9,
        .m13 = lhs.m13 + rhs.m13,
        .m2 = lhs.m2 + rhs.m2,
        .m6 = lhs.m6 + rhs.m6,
        .m10 = lhs.m10 + rhs.m10,
        .m14 = lhs.m14 + rhs.m14,
        .m3 = lhs.m3 + rhs.m3,
        .m7 = lhs.m7 + rhs.m7,
        .m11 = lhs.m11 + rhs.m11,
        .m15 = lhs.m15 + rhs.m15
    };

    return res;
}

static inline Matrix4 matrix4_sub(const Matrix4& lhs, const Matrix4& rhs) {
    Matrix4 res {
        .m0 = lhs.m0 - rhs.m0,
        .m4 = lhs.m4 - rhs.m4,
        .m8 = lhs.m8 - rhs.m8,
        .m12 = lhs.m12 - rhs.m12,
        .m1 = lhs.m1 - rhs.m1,
        .m5 = lhs.m5 - rhs.m5,
        .m9 = lhs.m9 - rhs.m9,
        .m13 = lhs.m13 - rhs.m13,
        .m2 = lhs.m2 - rhs.m2,
        .m6 = lhs.m6 - rhs.m6,
        .m10 = lhs.m10 - rhs.m10,
        .m14 = lhs.m14 - rhs.m14,
        .m3 = lhs.m3 - rhs.m3,
        .m7 = lhs.m7 - rhs.m7,
        .m11 = lhs.m11 - rhs.m11,
        .m15 = lhs.m15 - rhs.m15
    };

    return res;
}

static inline Matrix4 matrix4_mul(const Matrix4& lhs, const Matrix4& rhs) {
    Matrix4 res {
        .m0  = lhs.m0*rhs.m0  + lhs.m1*rhs.m4  + lhs.m2*rhs.m8   + lhs.m3*rhs.m12,
        .m1  = lhs.m0*rhs.m1  + lhs.m1*rhs.m5  + lhs.m2*rhs.m9   + lhs.m3*rhs.m13,
        .m2  = lhs.m0*rhs.m2  + lhs.m1*rhs.m6  + lhs.m2*rhs.m10  + lhs.m3*rhs.m14,
        .m3  = lhs.m0*rhs.m3  + lhs.m1*rhs.m7  + lhs.m2*rhs.m11  + lhs.m3*rhs.m15,
        .m4  = lhs.m4*rhs.m0  + lhs.m5*rhs.m4  + lhs.m6*rhs.m8   + lhs.m7*rhs.m12,
        .m5  = lhs.m4*rhs.m1  + lhs.m5*rhs.m5  + lhs.m6*rhs.m9   + lhs.m7*rhs.m13,
        .m6  = lhs.m4*rhs.m2  + lhs.m5*rhs.m6  + lhs.m6*rhs.m10  + lhs.m7*rhs.m14,
        .m7  = lhs.m4*rhs.m3  + lhs.m5*rhs.m7  + lhs.m6*rhs.m11  + lhs.m7*rhs.m15,
        .m8  = lhs.m8*rhs.m0  + lhs.m9*rhs.m4  + lhs.m10*rhs.m8  + lhs.m11*rhs.m12,
        .m9  = lhs.m8*rhs.m1  + lhs.m9*rhs.m5  + lhs.m10*rhs.m9  + lhs.m11*rhs.m13,
        .m10 = lhs.m8*rhs.m2  + lhs.m9*rhs.m6  + lhs.m10*rhs.m10 + lhs.m11*rhs.m14,
        .m11 = lhs.m8*rhs.m3  + lhs.m9*rhs.m7  + lhs.m10*rhs.m11 + lhs.m11*rhs.m15,
        .m12 = lhs.m12*rhs.m0 + lhs.m13*rhs.m4 + lhs.m14*rhs.m8  + lhs.m15*rhs.m12,
        .m13 = lhs.m12*rhs.m1 + lhs.m13*rhs.m5 + lhs.m14*rhs.m9  + lhs.m15*rhs.m13,
        .m14 = lhs.m12*rhs.m2 + lhs.m13*rhs.m6 + lhs.m14*rhs.m10 + lhs.m15*rhs.m14,
        .m15 = lhs.m12*rhs.m3 + lhs.m13*rhs.m7 + lhs.m14*rhs.m11 + lhs.m15*rhs.m15,
    };

    return res;
}

static inline float   matrix4_det(const Matrix4& mat) {
    float result = 0.0f;

    float m0  = mat.m0,  m1  = mat.m1,  m2  = mat.m2,  m3  = mat.m3;
    float m4  = mat.m4,  m5  = mat.m5,  m6  = mat.m6,  m7  = mat.m7;
    float m8  = mat.m8,  m9  = mat.m9,  m10 = mat.m10, m11 = mat.m11;
    float m12 = mat.m12, m13 = mat.m13, m14 = mat.m14, m15 = mat.m15;

    result = (m0 *((m5*(m10*m15 - m11*m14) - m9*(m6*m15 - m7*m14) + m13*(m6*m11 - m7*m10))) -
              m4 *((m1*(m10*m15 - m11*m14) - m9*(m2*m15 - m3*m14) + m13*(m2*m11 - m3*m10))) +
              m8 *((m1*(m6 *m15 - m7 *m14) - m5*(m2*m15 - m3*m14) + m13*(m2*m7  - m3*m6)))  -
              m12*((m1*(m6 *m11 - m7 *m10) - m5*(m2*m11 - m3*m10) + m9 *(m2*m7  - m3*m6))));

    return result;
}

static inline void matrix4_transpose(Matrix4& mat) {
    swap(&mat.m1,  &mat.m4);
    swap(&mat.m2,  &mat.m8);
    swap(&mat.m3,  &mat.m12);
    swap(&mat.m6,  &mat.m9);
    swap(&mat.m7,  &mat.m13);
    swap(&mat.m11, &mat.m14);

}

static inline Matrix4 matrix4_transposed(const Matrix4& mat) {
    Matrix4 res = {
        .m0  = mat.m0,
        .m4  = mat.m1,
        .m8  = mat.m2,
        .m12 = mat.m3,
        .m1  = mat.m4,
        .m5  = mat.m5,
        .m9  = mat.m6,
        .m13 = mat.m7,
        .m2  = mat.m8,
        .m6  = mat.m9,
        .m10 = mat.m10,
        .m14 = mat.m11,
        .m3  = mat.m12,
        .m7  = mat.m13,
        .m11 = mat.m14,
        .m15 = mat.m15
    };

    return res;
}

static inline Matrix4 matrix4_transform(const float x, const float y, const float z) {
    return matrix4_make(1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        x, y, z, 1);
}

static inline Matrix4 matrix4_transform(const Vector3 v) {
    return matrix4_make(1,   0,   0,   0,
                        0,   1,   0,   0,
                        0,   0,   1,   0,
                        v.x, v.y, v.z, 1);
}

static inline Matrix4 matrix4_transform_2d(const float x, const float y) {
    return matrix4_make(1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        x, y, 0, 1);
}

static inline Matrix4 matrix4_rotate_2d(float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    return matrix4_make( c, s, 0, 0,
                        -s, c, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1);
}

static inline Matrix4 matrix4_scale_2d(float x, float y) {
    return matrix4_make( x, 0, 0, 0,
                         0, y, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1);
}

static inline Matrix4 matrix4_scale(const float x, const float y, const float z) {
    return matrix4_make( x, 0, 0, 0,
                         0, y, 0, 0,
                         0, 0, z, 0,
                         0, 0, 0, 1);
}

static inline Matrix4 matrix4_scale(const Vector3 v) {
    return matrix4_make( v.x, 0,   0,   0,
                         0,   v.y, 0,   0,
                         0,   0,   v.z, 0,
                         0,   0,   0,   1);
}

static inline Matrix4 matrix4_trs_2d(float px, float py, float angle, float sx, float sy) {
    float s = sinf(angle);
    float c = cosf(angle);

    return matrix4_make( c * sx,  s * sx, 0, 0,
                        -s * sy,  c * sy, 0, 0,
                         0,       0,      1, 0,
                         px,      py,     0, 1);
}

static inline Matrix4 matrix4_camera_view_2d(Vector3 position) {
    return matrix4_make( 1,            0,          0, 0,
                         0,            1,          0, 0,
                         0,            0,          1, 0,
                         -position.x, -position.y, 0, 1);
}

static inline Matrix4 matrix4_ortho_2d(float left, float right, float top, float bottom) {
    float rml = right - left;
    float tmb = top - bottom;
    return matrix4_make( 2.0f / (rml),            0,                      0, 0,
                         0,                       -2.0f / (tmb),          0, 0,
                         0,                       0,                      1, 0,
                         -(right + left) / (rml), (top + bottom) / (tmb), 0, 1);
}

static inline Matrix4 matrix4_rotate(const Quaternion& q) {
    Matrix4 mat = matrix4_identity;

    float xx = q.x*q.x;
    float yy = q.y*q.y;
    float zz = q.z*q.z;
    float xy = q.x*q.y;
    float wz = q.w*q.z;
    float xz = q.x*q.z;
    float wy = q.w*q.y;
    float yz = q.y*q.z;
    float wx = q.w*q.x;

    mat.m0  = 1.0f    - 2.0f*yy  - 2.0f*zz;
    mat.m1  = 2.0f*(xy - wz);
    mat.m2  = 2.0f*(xz + wy);
    mat.m3  = 0.0f;

    mat.m4  = 2.0f*(xy + wz);
    mat.m5  = 1.0f    - 2.0f*xx  - 2.0f*zz;
    mat.m6  = 2.0f*(yz - wx);
    mat.m7  = 0.0f;

    mat.m8  = 2.0f*(xz - wy);
    mat.m9  = 2.0f*(yz + wx);
    mat.m10 = 1.0f    - 2.0f*xx  - 2.0f*yy;
    mat.m11 = 0.0f;

    mat.m12 = 0.0f;
    mat.m13 = 0.0f;
    mat.m14 = 0.0f;
    mat.m15 = 1.0f;

    // mat.m0  = 1.0f    - 2.0f*yy  - 2.0f*zz;
    // mat.m1  = 2.0f*xy + 2.0f*wz;
    // mat.m2  = 2.0f*xz - 2.0f*wy;
    // mat.m3  = 0.0f;

    // mat.m4  = 2.0f*xy - 2.0f*wz;
    // mat.m5  = 1.0f    - 2.0f*xx  - 2.0f*zz;
    // mat.m6  = 2.0f*yz + 2.0f*wx;
    // mat.m7  = 0.0f;

    // mat.m8  = 2.0f*xz + 2.0f*wy;
    // mat.m9  = 2.0f*yz - 2.0f*wx;
    // mat.m10 = 1.0f    - 2.0f*xx  - 2.0f*yy;
    // mat.m11 = 0.0f;

    // mat.m12 = 0.0f;
    // mat.m13 = 0.0f;
    // mat.m14 = 0.0f;
    // mat.m15 = 1.0f;

    return mat;
}

static inline Matrix4 matrix4_trs(const Vector3& p, const Quaternion& r, const Vector3& s) {
    Matrix4 t = matrix4_transform(p);
    Matrix4 rot = matrix4_rotate(r);
    Matrix4 sc  = matrix4_scale(s);

    Matrix4 res = sc * rot;

    res = res * t;

    return res;

    // return t * rot * sc;
}

static inline Matrix4 matrix4_perspective(float fov,
                                          float aspect,
                                          float near_plane,
                                          float far_plane) {
    Matrix4 proj = matrix4_identity;

    float t = tanf(fov * 0.5f);

    proj.m0 = 1.0f / (aspect * t);
    proj.m5 = 1.0f / t;
    proj.m10 = ((far_plane + near_plane) / (far_plane - near_plane));
    proj.m11 = -((2.0f*far_plane*near_plane) / (far_plane - near_plane));
    proj.m14 = 1.0f;
    proj.m15 = 0.0f;

    return proj;
}

static inline Matrix4 matrix4_view(const Vector3& p, const Quaternion& r) {
    Matrix4 t   = matrix4_transform(-p);
    Matrix4 rot = matrix4_rotate(conjugate(r));

    Matrix4 view = rot * t;

    return view;
}

static inline Matrix4 matrix4_vp(const Vector3& eye_pos, 
                                 const Quaternion& r,
                                 float fov,
                                 float aspect,
                                 float near_plane,
                                 float far_plane) {
    Matrix4 view = matrix4_view(eye_pos, r);
    Matrix4 proj = matrix4_perspective(fov, aspect, near_plane, far_plane);

    return proj * view;
}

static inline Matrix4 matrix4_mvp(const Vector3& p, 
                                  const Quaternion& r, 
                                  const Vector3& s,
                                  const Vector3& eye_p,
                                  const Quaternion& eye_r,
                                  float fov,
                                  float aspect,
                                  float near_plane,
                                  float far_plane) {
    Matrix4 model = matrix4_trs(p, r, s);
    Matrix4 vp    = matrix4_vp(eye_p, eye_r, fov, aspect, near_plane, far_plane);

    return vp * model;
}

static inline void matrix4_print(Matrix4& m) {
    Logf("%f, %f, %f, %f\n"
         "%f, %f, %f, %f\n"
         "%f, %f, %f, %f\n"
         "%f, %f, %f, %f\n", m.m0, m.m4, m.m8,  m.m12,
                             m.m1, m.m5, m.m9,  m.m13,
                             m.m2, m.m6, m.m10, m.m14,
                             m.m3, m.m7, m.m11, m.m15);
}

#endif //GAME_MATH_IMPLEMENTATION