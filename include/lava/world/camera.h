#ifndef LAVA_WORLD_CAMERA_H
#define LAVA_WORLD_CAMERA_H

#include "lava/internal.h"
#include "lava/math/vector.h"
#include "lava/math/matrix.h"


typedef enum {
    lvCameraProjection_PERSPECTIVE,
    lvCameraProjection_ORTHOGRAPHIC
} lvCameraProjection;


typedef enum {
    lvCameraMode_FIRST_PERSON,
    lvCameraMode_ORBIT
} lvCameraMode;


typedef struct {
    lvCameraProjection projection;
    lvCameraMode mode;

    lvMatrix4 proj_mat;
    lvMatrix4 view_mat;

    lvVector3 position;
    lvVector3 front;
    lvVector3 up;

    float yaw;
    float pitch;

    // Only for lvCameraMode_ORBIT mode
    lvVector3 target;
    float distance;
} lvCamera;

lvCamera lvCamera_new_perspective(
    float aspect_ratio,
    float near_z,
    float far_z,
    float fov
);

void lvCamera_update(lvCamera *camera);

void lvCamera_move(lvCamera *camera, float amount);

void lvCamera_strafe(lvCamera *camera, float amount);


#endif // LAVA_WORLD_CAMERA_H