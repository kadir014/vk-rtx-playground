#ifndef LAVA_WORLD_MESH_H
#define LAVA_WORLD_MESH_H

#include "lava/internal.h"
#include "lava/vk/buffer.h"
#include "lava/vk/context.h"
#include "lava/loaders/obj.h"


typedef struct {
    size_t n_vertices;
    lvVector3 *vertices_arr;
    lvBuffer vertices;
    lvBuffer normals;
    lvBuffer uvs;
} lvMesh;

int lvMesh_init(lvMesh *mesh, lvContext *ctx, lvOBJMesh obj_mesh);

void lvMesh_free(lvMesh *mesh, lvContext *ctx);


#endif // LAVA_WORLD_MESH_H