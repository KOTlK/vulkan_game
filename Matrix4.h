#pragma once

#include <math.h>

#define degrees(rad) rad * 57.2957795f
#define radians(deg) deg * 0.0174532925f

typedef union Matrix4 {
    struct {
        float m00, m10, m20, m30;
        float m01, m11, m21, m31;
        float m02, m12, m22, m32;
        float m03, m13, m23, m33;
    };
    float e[16];
} Matrix4;

static inline Matrix4 matrix4_make(float m00, float m10, float m20, float m30,
                                   float m01, float m11, float m21, float m31,
                                   float m02, float m12, float m22, float m32,
                                   float m03, float m13, float m23, float m33);

static inline Matrix4 matrix4_add(Matrix4* lhs, Matrix4* rhs);
static inline Matrix4 matrix4_sub(Matrix4* lhs, Matrix4* rhs);
static inline Matrix4 matrix4_mul(Matrix4* lhs, Matrix4* rhs);
static inline Matrix4 matrix4_transform_2d(float x, float y);
static inline Matrix4 matrix4_rotate_2d(float angle);
static inline Matrix4 matrix4_scale_2d(float x, float y);
static inline Matrix4 matrix4_trs_2d(float px, float py, float angle, float sx, float sy);
static inline Matrix4 matrix4_ortho_2d(float left, float right, float top, float bottom);
static inline Matrix4 matrix4_mvp(float cx, float cy, float left, float right, float top, float bottom, float px, float py, float r, float sx, float sy);
static inline float   matrix4_det(Matrix4* mat);

#ifdef GAME_MATH_IMPLEMENTATION
static inline Matrix4 matrix4_make(float m00, float m10, float m20, float m30,
                                   float m01, float m11, float m21, float m31,
                                   float m02, float m12, float m22, float m32,
                                   float m03, float m13, float m23, float m33) {
    Matrix4 matrix = {
        .m00 = m00,
        .m10 = m10,
        .m20 = m20,
        .m30 = m30,
        .m01 = m01,
        .m11 = m11,
        .m21 = m21,
        .m31 = m31,
        .m02 = m02,
        .m12 = m12,
        .m22 = m22,
        .m32 = m32,
        .m03 = m03,
        .m13 = m13,
        .m23 = m23,
        .m33 = m33
    };

    return matrix;
}

static inline Matrix4 matrix4_add(Matrix4* lhs, Matrix4* rhs) {
    Matrix4 res {
        .m00 = lhs->m00 + rhs->m00,
        .m10 = lhs->m10 + rhs->m10,
        .m20 = lhs->m20 + rhs->m20,
        .m30 = lhs->m30 + rhs->m30,
        .m01 = lhs->m01 + rhs->m01,
        .m11 = lhs->m11 + rhs->m11,
        .m21 = lhs->m21 + rhs->m21,
        .m31 = lhs->m31 + rhs->m31,
        .m02 = lhs->m02 + rhs->m02,
        .m12 = lhs->m12 + rhs->m12,
        .m22 = lhs->m22 + rhs->m22,
        .m32 = lhs->m32 + rhs->m32,
        .m03 = lhs->m03 + rhs->m03,
        .m13 = lhs->m13 + rhs->m13,
        .m23 = lhs->m23 + rhs->m23,
        .m33 = lhs->m33 + rhs->m33
    };

    return res;
}

static inline Matrix4 matrix4_sub(Matrix4* lhs, Matrix4* rhs) {
    Matrix4 res {
        .m00 = lhs->m00 - rhs->m00,
        .m10 = lhs->m10 - rhs->m10,
        .m20 = lhs->m20 - rhs->m20,
        .m30 = lhs->m30 - rhs->m30,
        .m01 = lhs->m01 - rhs->m01,
        .m11 = lhs->m11 - rhs->m11,
        .m21 = lhs->m21 - rhs->m21,
        .m31 = lhs->m31 - rhs->m31,
        .m02 = lhs->m02 - rhs->m02,
        .m12 = lhs->m12 - rhs->m12,
        .m22 = lhs->m22 - rhs->m22,
        .m32 = lhs->m32 - rhs->m32,
        .m03 = lhs->m03 - rhs->m03,
        .m13 = lhs->m13 - rhs->m13,
        .m23 = lhs->m23 - rhs->m23,
        .m33 = lhs->m33 - rhs->m33
    };

    return res;
}

static inline Matrix4 matrix4_mul(Matrix4* lhs, Matrix4* rhs) {
    Matrix4 res {
        .m00 = lhs->m00 * rhs->m00 + lhs->m01 * rhs->m10 + lhs->m02 * rhs->m20 + lhs->m03 * rhs->m30,
        .m01 = lhs->m00 * rhs->m01 + lhs->m01 * rhs->m11 + lhs->m02 * rhs->m21 + lhs->m03 * rhs->m31,
        .m02 = lhs->m00 * rhs->m02 + lhs->m01 * rhs->m12 + lhs->m02 * rhs->m22 + lhs->m03 * rhs->m32,
        .m03 = lhs->m00 * rhs->m03 + lhs->m01 * rhs->m13 + lhs->m02 * rhs->m23 + lhs->m03 * rhs->m33,
        .m10 = lhs->m10 * rhs->m00 + lhs->m11 * rhs->m10 + lhs->m12 * rhs->m20 + lhs->m13 * rhs->m30,
        .m11 = lhs->m10 * rhs->m01 + lhs->m11 * rhs->m11 + lhs->m12 * rhs->m21 + lhs->m13 * rhs->m31,
        .m12 = lhs->m10 * rhs->m02 + lhs->m11 * rhs->m12 + lhs->m12 * rhs->m22 + lhs->m13 * rhs->m32,
        .m13 = lhs->m10 * rhs->m03 + lhs->m11 * rhs->m13 + lhs->m12 * rhs->m23 + lhs->m13 * rhs->m33,
        .m20 = lhs->m20 * rhs->m00 + lhs->m21 * rhs->m10 + lhs->m22 * rhs->m20 + lhs->m23 * rhs->m30,
        .m21 = lhs->m20 * rhs->m01 + lhs->m21 * rhs->m11 + lhs->m22 * rhs->m21 + lhs->m23 * rhs->m31,
        .m22 = lhs->m20 * rhs->m02 + lhs->m21 * rhs->m12 + lhs->m22 * rhs->m22 + lhs->m23 * rhs->m32,
        .m23 = lhs->m20 * rhs->m03 + lhs->m21 * rhs->m13 + lhs->m22 * rhs->m23 + lhs->m23 * rhs->m33,
        .m30 = lhs->m30 * rhs->m00 + lhs->m31 * rhs->m10 + lhs->m32 * rhs->m20 + lhs->m33 * rhs->m30,
        .m31 = lhs->m30 * rhs->m01 + lhs->m31 * rhs->m11 + lhs->m32 * rhs->m21 + lhs->m33 * rhs->m31,
        .m32 = lhs->m30 * rhs->m02 + lhs->m31 * rhs->m12 + lhs->m32 * rhs->m22 + lhs->m33 * rhs->m32,
        .m33 = lhs->m30 * rhs->m03 + lhs->m31 * rhs->m13 + lhs->m32 * rhs->m23 + lhs->m33 * rhs->m33,
    };

    return res;
}

static inline float   matrix4_det(Matrix4* mat) {
    float result = 0.0f;

    float m0  = mat->m00, m1  = mat->m01, m2  = mat->m02, m3  = mat->m03;
    float m4  = mat->m10, m5  = mat->m11, m6  = mat->m12, m7  = mat->m13;
    float m8  = mat->m20, m9  = mat->m21, m10 = mat->m22, m11 = mat->m23;
    float m12 = mat->m30, m13 = mat->m31, m14 = mat->m32, m15 = mat->m33;

    result = (m0 *((m5*(m10*m15 - m11*m14) - m9*(m6*m15 - m7*m14) + m13*(m6*m11 - m7*m10))) -
              m4 *((m1*(m10*m15 - m11*m14) - m9*(m2*m15 - m3*m14) + m13*(m2*m11 - m3*m10))) +
              m8 *((m1*(m6 *m15 - m7 *m14) - m5*(m2*m15 - m3*m14) + m13*(m2*m7  - m3*m6)))  -
              m12*((m1*(m6 *m11 - m7 *m10) - m5*(m2*m11 - m3*m10) + m9 *(m2*m7  - m3*m6))));

    return result;
}

static inline Matrix4 matrix4_transform_2d(float x, float y) {
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

static inline Matrix4 matrix4_trs_2d(float px, float py, float angle, float sx, float sy) {
    float s = sinf(angle);
    float c = cosf(angle);

    return matrix4_make( c * sx,  s * sx, 0, 0,
                        -s * sy,  c * sy, 0, 0,
                         0,       0,      1, 0,
                         px,      py,     0, 1);
}

static inline Matrix4 matrix4_ortho_2d(float left, float right, float top, float bottom) {
    float rml = right - left;
    float tmb = top - bottom;
    return matrix4_make( 2.0f / (rml),            0,                       0, 0,
                         0,                       -2.0f / (tmb),            0, 0,
                         0,                       0,                       1, 0,
                         -(right + left) / (rml), (top + bottom) / (tmb), 0, 1);
}

static inline Matrix4 matrix4_mvp(float cx, 
                                  float cy, 
                                  float l, 
                                  float r, 
                                  float t,
                                  float b,
                                  float px,
                                  float py,
                                  float angle,
                                  float sx,
                                  float sy) {
    float c   = cosf(angle);
    float s   = sinf(angle);
    float rml = r-l;
    float tmb = t-b;

    float div1 = 2.0f / rml;
    float div2 = 2.0f / tmb;

    return matrix4_make(div1 * c * sx,  -div2 * s * sx,  0, 0,
                        -div1 * s * sy, -div2 * c * sy,  0, 0,
                        0,               0,              1, 0,

    ((2.0f * (px-cx)) - (r+l)) / rml, ((-2.0f * (py+cy)) + (t+b)) / tmb, 0, 1);
}

#endif //GAME_MATH_IMPLEMENTATION