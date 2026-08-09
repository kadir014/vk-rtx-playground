#ifndef LAVA_WORLD_MODEL_H
#define LAVA_WORLD_MODEL_H

#include "lava/internal.h"
#include "lava/containers/refarray.h"
#include "lava/world/mesh.h"
#include "lava/math/transform.h"
#include "lava/world/material.h"



#define LV_MODEL_NAME_LENGTH 16


typedef struct {
    char name[LV_MODEL_NAME_LENGTH];
    lvTransform xform;
    lvRefArray meshes;
    char material_name[LV_MATERIAL_NAME_LENGTH];
} lvModel;


#endif // LAVA_WORLD_MODEL_H