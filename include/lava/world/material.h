#ifndef LAVA_WORLD_MATERIAL_H
#define LAVA_WORLD_MATERIAL_H

#include "lava/internal.h"
#include "lava/vk/image.h"


#define LV_MATERIAL_NAME_LENGTH 16


typedef struct {
    char name[LV_MATERIAL_NAME_LENGTH];
    lvImage image;
} lvMaterial;


#endif // LAVA_WORLD_MATERIAL_H