#ifndef LAVA_WORLD_CAMERA_H
#define LAVA_WORLD_CAMERA_H

#include "lava/internal.h"


typedef struct {
    vec3 position;
    vec3 dir;
    vec3 up;
    float fov;
    float near_z;
    float far_z;
} lvCamera;


#endif // LAVA_WORLD_CAMERA_H