#ifndef LAVA_MATH_TRANSFORM_H
#define LAVA_MATH_TRANSFORM_H

#include "lava/math/math.h"
#include "lava/math/vector.h"
#include "lava/math/matrix.h"


/**
 * @brief 3D transformation in space.
 */
typedef struct {
    lvVector3 position; /**< Translation of the transform. */
    lvVector3 rotation; /**< Rotation of the transform as euler angles. */
    lvVector3 scale; /**< Scale of the transform. */
} lvTransform;

static const lvTransform lvTransform_identity = {
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f}
};

static inline lvMatrix4 lvTransform_to_matrix4(lvTransform xform) {
    lvMatrix4 mat = lvMatrix4_identity;

    // Rotation -> Scale -> Translation

    // Prepare sin and cos for euler angles
    float cx = lv_cosf(xform.rotation.x);
    float sx = lv_sinf(xform.rotation.x);
    float cy = lv_cosf(xform.rotation.y);
    float sy = lv_sinf(xform.rotation.y);
    float cz = lv_cosf(xform.rotation.z);
    float sz = lv_sinf(xform.rotation.z);

    // Rotation order: Z * Y * X
    mat.m[0] = cy * cz;
    mat.m[4] = -cy * sz;
    mat.m[8] = sy;

    mat.m[1] = sx * sy * cz + cx * sz;
    mat.m[5] = -sx * sy * sz + cx * cz;
    mat.m[9] = -sx * cy;

    mat.m[2] = -cx * sy * cz + sx * sz;
    mat.m[6] = cx * sy * sz + sx * cz;
    mat.m[10] = cx * cy;

    // Apply scale
    mat.m[0] *= xform.scale.x;
    mat.m[1] *= xform.scale.x;
    mat.m[2] *= xform.scale.x;

    mat.m[4] *= xform.scale.y;
    mat.m[5] *= xform.scale.y;
    mat.m[6] *= xform.scale.y;

    mat.m[8] *= xform.scale.z;
    mat.m[9] *= xform.scale.z;
    mat.m[10] *= xform.scale.z;

    // Translate
    mat.m[12] = xform.position.x;
    mat.m[13] = xform.position.y;
    mat.m[14] = xform.position.z;

    return mat;
}


#endif // LAVA_MATH_TRANSFORM_H