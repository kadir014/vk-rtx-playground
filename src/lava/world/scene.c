#include "lava/world/scene.h"
#include "lava/world/model.h"


lvScene lvScene_new(lvCamera *camera) {
    lvScene scene = {0};

    scene.camera = camera;

    scene.models = lvArray_new(sizeof(lvModel));

    scene.materials = lvRefArray_new();

    return scene;
}

void lvScene_free(lvScene *scene, lvContext *ctx) {
    if (!scene) return;

    for (size_t i = 0; i < scene->models.size; i++) {
        lvModel *model = LV_ARRAY_PTR_AT(&scene->models, i, lvModel);

        for (size_t i = 0; i < model->meshes.size; i++) {
            lvMesh *mesh = model->meshes.data[i];
            lvMesh_free(mesh, ctx);
            LV_FREE(mesh);
        }
        lvRefArray_free(&model->meshes);
        lvRefArray_free(&model->materials);
    }

    lvArray_free(&scene->models);

    lvRefArray_free(&scene->materials);
}

int lvScene_add_model(
    lvScene *scene,
    lvContext *ctx,
    lvOBJ *obj,
    const char name[LV_MODEL_NAME_LENGTH]
) {
    if (!scene || !ctx || !obj || !name) return 1;

    lvModel model = {0};
    memcpy(model.name, name, sizeof(char) * LV_MODEL_NAME_LENGTH);
    model.xform = (lvTransform){
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f}
    };
    model.meshes = lvRefArray_new();
    if (!lvRefArray_valid(&model.meshes)) return 1;
    model.materials = lvRefArray_new();
    if (!lvRefArray_valid(&model.materials)) return 1;

    //for (size_t i = 0; i < obj->meshes; i++)

    lvMesh *mesh = LV_MALLOC(sizeof(lvMesh));
    if (lvMesh_init(mesh, ctx, obj->mesh) != 0) return 1;

    if (lvRefArray_add(&model.meshes, mesh) != 0) return 1;

    if (lvArray_add(&scene->models, &model) != 0) return 1;

    return 0;
}

lvModel *lvScene_get_model(lvScene *scene, const char *name) {
    for (size_t i = 0; i < scene->models.size; i++) {
        lvModel *model = LV_ARRAY_PTR_AT(&scene->models, i, lvModel);

        if (strcmp(model->name, name) == 0) {
            return model;
        }
    }

    return NULL;
}