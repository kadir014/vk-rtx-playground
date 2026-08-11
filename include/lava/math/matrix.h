#ifndef LAVA_MATH_MATRIX_H
#define LAVA_MATH_MATRIX_H

#include "lava/math/math.h"
#include "lava/math/vector.h"


/**
 * @brief 4x4 matrix in column-major order.
 */
typedef struct {
    float m[16];
} lvMatrix4;


/**
 * @brief Constant 4x4 identity matrix.
 */
static const lvMatrix4 lvMatrix4_identity = {
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }
};

/**
 * @brief Constant 4x4 zero matrix.
 */
static const lvMatrix4 lvMatrix4_zero = {
    {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    }
};


/**
 * @brief Inline 4x4 matrix initialization.
 * 
 * @param m Array of elements in column-major order.
 * @return lvMatrix4
 */
static inline lvMatrix4 lv_matrix4(float m[16]) {
    return (lvMatrix4){{
        m[0],  m[1],  m[2],  m[3],
        m[4],  m[5],  m[6],  m[7],
        m[8],  m[9],  m[10], m[11],
        m[12], m[13], m[14], m[15]
    }};
}

static inline lvMatrix4 lv_matrix4_s(float s) {
    return (lvMatrix4){{
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
        s, s, s, s
    }};
}


#define lvMatrix4_get(mat, row, col) (mat.m[(col) * 4 + (row)])

#define lvMatrix4_set(mat, row, col, value) (mat.m[(col) * 4 + (row)] = (value))


/**
 * @brief Multiply two 4x4 matrices.
 * 
 * @param a Left-hand matrix.
 * @param b Right-hand matrix.
 * @return lvMatrix4
 */
static inline lvMatrix4 lvMatrix4_mul(lvMatrix4 a, lvMatrix4 b) {
    lvMatrix4 result = lvMatrix4_zero;

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            result.m[col * 4 + row] = sum;
        }
    }

    return result;
}

static inline lvMatrix4 lvMatrix4_perspective(
    float fov,
    float aspect,
    float z_near,
    float z_far,
    float vert_scale
) {
    lvMatrix4 mat = lvMatrix4_zero;

    float tan_half = lv_tanf(fov * 0.5f);

    mat.m[0] = 1.0f / (aspect * tan_half);
    mat.m[5] = 1.0f / tan_half * vert_scale;
    mat.m[10] = -(z_far + z_near) / (z_far - z_near);
    mat.m[11] = -1.0f;
    mat.m[14] = -(2.0f * z_far * z_near) / (z_far - z_near);

    return mat;
}

static inline lvMatrix4 lvMatrix4_translate(lvMatrix4 mat, lvVector3 vec) {
    mat.m[12] += vec.x;
    mat.m[13] += vec.y;
    mat.m[14] += vec.z;
    return mat;
}

static inline lvMatrix4 lvMatrix4_look_at(
    lvVector3 position,
    lvVector3 target,
    lvVector3 up
) {
    lvVector3 f = lvVector3_normalize(lvVector3_sub(target, position));
    lvVector3 s = lvVector3_normalize(lvVector3_cross(f, up));
    lvVector3 u = lvVector3_cross(s, f);

    lvMatrix4 result = lvMatrix4_identity;
    result.m[0] = s.x;  result.m[4] = s.y;  result.m[8] = s.z;   result.m[12] = -lvVector3_dot(s, position);
    result.m[1] = u.x;  result.m[5] = u.y;  result.m[9] = u.z;   result.m[13] = -lvVector3_dot(u, position);
    result.m[2] = -f.x; result.m[6] = -f.y; result.m[10] = -f.z; result.m[14] = lvVector3_dot(f, position);

    return result;
}


#endif // LAVA_MATH_MATRIX_H