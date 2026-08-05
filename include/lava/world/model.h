#ifndef LAVA_WORLD_MODEL_H
#define LAVA_WORLD_MODEL_H

#include "lava/internal.h"
#include "lava/containers/refarray.h"
#include "lava/world/mesh.h"
#include "lava/world/transform.h"
#include "lava/world/material.h"


typedef struct {
    lvTransform xform;
    lvRefArray meshes;
    lvRefArray materials;
} lvModel;


#endif // LAVA_WORLD_MODEL_H