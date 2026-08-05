#ifndef LAVA_WORLD_SCENE_H
#define LAVA_WORLD_SCENE_H

#include "lava/internal.h"
#include "lava/containers/array.h"
#include "lava/world/camera.h"
#include "lava/world/model.h"
#include "lava/vk/context.h"
#include "lava/loaders/obj.h"


#define LV_SCENE_MODEL_NAME_LENGTH 64


typedef struct {
    lvCamera *camera;
    // Use a hashmap :(
    lvArray model_names;
    lvArray models;
    lvRefArray materials;
} lvScene;

lvScene lvScene_new(lvCamera *camera);

void lvScene_free(lvScene *scene, lvContext *ctx);

int lvScene_add_model(
    lvScene *scene,
    lvContext *ctx,
    lvOBJ *obj,
    const char name[LV_SCENE_MODEL_NAME_LENGTH]
);

lvModel *lvScene_get_model(lvScene *scene, const char *name);


#endif // LAVA_WORLD_SCENE_H