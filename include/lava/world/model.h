#ifndef LAVA_WORLD_MODEL_H
#define LAVA_WORLD_MODEL_H

#include "lava/internal.h"
#include "lava/containers/refarray.h"
#include "lava/world/mesh.h"
#include "lava/world/transform.h"
#include "lava/world/material.h"



#define LV_MODEL_NAME_LENGTH 64


typedef struct {
    char name[LV_MODEL_NAME_LENGTH];
    lvTransform xform;
    lvRefArray meshes;
    lvRefArray materials;
} lvModel;


#endif // LAVA_WORLD_MODEL_H