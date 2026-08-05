#ifndef LAVA_WORLD_SCENE_H
#define LAVA_WORLD_SCENE_H

#include "lava/internal.h"
#include "lava/containers/array.h"
#include "lava/world/camera.h"
#include "lava/world/model.h"
#include "lava/world/material.h"
#include "lava/vk/context.h"
#include "lava/loaders/obj.h"


typedef struct {
    lvCamera *camera;
    lvArray models;
    lvArray materials;
} lvScene;

lvScene lvScene_new(lvCamera *camera);

void lvScene_free(lvScene *scene, lvContext *ctx);

int lvScene_add_model(
    lvScene *scene,
    lvContext *ctx,
    lvOBJ *obj,
    const char name[LV_MODEL_NAME_LENGTH],
    const char material_name[LV_MATERIAL_NAME_LENGTH]
);

lvModel *lvScene_get_model(lvScene *scene, const char *name);

int lvScene_add_material(
    lvScene *scene,
    lvContext *ctx,
    lvImage image,
    const char name[LV_MATERIAL_NAME_LENGTH]
);

lvMaterial *lvScene_get_material(lvScene *scene, const char *name);


#endif // LAVA_WORLD_SCENE_H