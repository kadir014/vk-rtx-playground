#ifndef LAVA_MATH_VECTOR_H
#define LAVA_MATH_VECTOR_H

#include "lava/math/math.h"

/**
 * @brief 3D vector type.
 */
typedef struct {
    float x; /**< X component of the vector. */
    float y; /**< Y component of the vector. */
    float z; /**< Z component of the vector. */
} lvVector3;

/**
 * @brief 2D vector type.
 */
typedef struct {
    float x; /**< X component of the vector. */
    float y; /**< Y component of the vector. */
} lvVector2;


static const lvVector3 lvVector3_zero = {0.0f, 0.0f, 0.0f};

static const lvVector3 lvVector3_one = {1.0f, 1.0f, 1.0f};

/**
 * @brief Inline initialization for 3D vector.
 */
static inline lvVector3 lv_vector3(float x, float y, float z) {
    return (lvVector3){x, y, z};
}

/**
 * @brief Inline initialization for 3D vector from single scalar.
 */
static inline lvVector3 lv_vector3_s(float s) {
    return (lvVector3){s, s, s};
}

/**
 * @brief Inline initialization for 3D vector from 2D vector.
 * 
 * Z component is initialized as zero.
 */
static inline lvVector3 lv_vector3_v2(lvVector2 v) {
    return (lvVector3){v.x, v.y, 0.0f};
}

static inline lvVector3 lvVector3_add(lvVector3 a, lvVector3 b) {
    return lv_vector3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline lvVector3 lvVector3_sub(lvVector3 a, lvVector3 b) {
    return lv_vector3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline lvVector3 lvVector3_mul(lvVector3 v, float s) {
    return lv_vector3(v.x * s, v.y * s, v.z * s);
}

static inline lvVector3 lvVector3_div(lvVector3 v, float s) {
    return lv_vector3(v.x / s, v.y / s, v.z / s);
}

static inline lvVector3 nvVector2_neg(lvVector3 v) {
    return lv_vector3(-v.x, -v.y, -v.z);
}

static inline float lvVector3_len2(lvVector3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

static inline float lvVector3_len(lvVector3 v) {
    return lv_sqrtf(lvVector3_len2(v));
}

static inline lvVector3 lvVector3_normalize(lvVector3 v) {
    return lvVector3_div(v, lvVector3_len(v));
}

static inline float lvVector3_dot(lvVector3 a, lvVector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline lvVector3 lvVector3_cross(lvVector3 a, lvVector3 b) {
    return lv_vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}


static const lvVector2 lvVector2_zero = {0.0f, 0.0f};

static const lvVector2 lvVector2_one = {1.0f, 1.0f};

/**
 * @brief Inline initialization for 2D vector.
 */
static inline lvVector2 lv_vector2(float x, float y) {
    return (lvVector2){x, y};
}

/**
 * @brief Inline initialization for 2D vector from single scalar.
 */
static inline lvVector2 lv_vector2_s(float s) {
    return (lvVector2){s, s};
}

/**
 * @brief Inline initialization for 2D vector from 3D vector.
 * 
 * Only X and Y components of the 3D vector and Z component is ignored.
 */
static inline lvVector2 lv_vector2_v3(lvVector3 v) {
    return (lvVector2){v.x, v.y};
}



#endif // LAVA_MATH_VECTOR_H