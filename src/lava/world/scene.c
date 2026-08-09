#include "lava/world/scene.h"
#include "lava/world/model.h"


lvScene lvScene_new(lvCamera *camera) {
    lvScene scene = {0};

    scene.camera = camera;

    scene.models = lvArray_new(sizeof(lvModel));

    scene.materials = lvArray_new(sizeof(lvMaterial));

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
    }

    lvArray_free(&scene->models);

    lvArray_free(&scene->materials);
}

int lvScene_add_model(
    lvScene *scene,
    lvContext *ctx,
    lvOBJ *obj,
    const char name[LV_MODEL_NAME_LENGTH],
    const char material_name[LV_MATERIAL_NAME_LENGTH]
) {
    if (!scene || !ctx || !obj || !name) return 1;

    lvModel model = {0};

    memcpy(model.name, name, sizeof(char) * LV_MODEL_NAME_LENGTH);

    if (material_name) {
        memcpy(model.material_name, material_name, sizeof(char) * LV_MATERIAL_NAME_LENGTH);
    }

    model.xform = lvTransform_identity;

    model.meshes = lvRefArray_new();
    if (!lvRefArray_valid(&model.meshes)) return 1;

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

int lvScene_add_material(
    lvScene *scene,
    lvContext *ctx,
    lvImage image,
    const char name[LV_MATERIAL_NAME_LENGTH]
) {
    if (!scene || !ctx || !name) return 1;

    lvMaterial material = {0};

    memcpy(material.name, name, sizeof(char) * LV_MATERIAL_NAME_LENGTH);

    material.image = image;

    if (lvArray_add(&scene->materials, &material) != 0) return 1;

    return 0;
}

lvMaterial *lvScene_get_material(lvScene *scene, const char *name) {
    for (size_t i = 0; i < scene->materials.size; i++) {
        lvMaterial *material = LV_ARRAY_PTR_AT(&scene->materials, i, lvMaterial);

        if (strcmp(material->name, name) == 0) {
            return material;
        }
    }

    return NULL;
}