#include "lava/world/mesh.h"
#include "lava/containers/array.h"
#include "lava/math/vector.h"


int lvMesh_init(lvMesh *mesh, lvContext *ctx, lvOBJMesh obj_mesh) {
    if (obj_mesh.tris.size == 0 || !lvArray_valid(&obj_mesh.tris)) {
        return 1;
    }

    mesh->n_vertices = obj_mesh.tris.size * 3;

    {
        lvVector3 *vertices = LV_MALLOC(sizeof(lvVector3) * mesh->n_vertices);
        if (!vertices) {
            printf("Failed to allocate.\n");
            return 1;
        }

        size_t j = 0;
        for (size_t tri_idx = 0; tri_idx < obj_mesh.tris.size; tri_idx++) {
            lvOBJTri tri = LV_ARRAY_AT(&obj_mesh.tris, tri_idx, lvOBJTri);

            for (size_t i = 0; i < 3; i++) {
                vertices[j].x = tri.vertices[i].x;
                vertices[j].y = tri.vertices[i].y;
                vertices[j].z = tri.vertices[i].z;
                j += 1;
            }
        }

        mesh->vertices = (lvBuffer){
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .stride = sizeof(lvVector3),
        };
        if (lvBuffer_init_vertex(&mesh->vertices, ctx, sizeof(lvVector3) * mesh->n_vertices, 0) != 0) {
            printf("Buffer creation failed.\n");
            return 1;
        }

        void *vertex_buffer_data;
        if (vmaMapMemory(ctx->allocator, mesh->vertices._allocation, &vertex_buffer_data) != VK_SUCCESS) {
            printf("Memory mapping failed.\n");
            return 1;
        }
        memcpy(vertex_buffer_data, vertices, sizeof(lvVector3) * mesh->n_vertices);
        vmaUnmapMemory(ctx->allocator, mesh->vertices._allocation);

        LV_FREE(vertices);
    }

    {
        lvVector2 *uvs = LV_MALLOC(sizeof(lvVector2) * mesh->n_vertices);
        size_t j = 0;
        for (size_t tri_idx = 0; tri_idx < obj_mesh.tris.size; tri_idx++) {
            lvOBJTri tri = LV_ARRAY_AT(&obj_mesh.tris, tri_idx, lvOBJTri);

            for (size_t i = 0; i < 3; i++) {
                uvs[j].x = tri.uvs[i].x;
                uvs[j].y = tri.uvs[i].y;
                j += 1;
            }
        }

        mesh->uvs = (lvBuffer){
            .location = 1,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .stride = sizeof(lvVector2),
        };
        if (lvBuffer_init_vertex(&mesh->uvs, ctx, sizeof(lvVector2) * mesh->n_vertices, 1) != 0) {
            printf("Buffer creation failed.\n");
            return 1;
        }

        void *uv_buffer_data;
        if (vmaMapMemory(ctx->allocator, mesh->uvs._allocation, &uv_buffer_data) != VK_SUCCESS) {
            printf("Memory mapping failed.\n");
            return 1;
        }
        memcpy(uv_buffer_data, uvs, sizeof(lvVector2) * mesh->n_vertices);
        vmaUnmapMemory(ctx->allocator, mesh->uvs._allocation);

        LV_FREE(uvs);
    }

    {
        lvVector3 *normals = LV_MALLOC(sizeof(lvVector3) * mesh->n_vertices);
        size_t j = 0;
        for (size_t tri_idx = 0; tri_idx < obj_mesh.tris.size; tri_idx++) {
            lvOBJTri tri = LV_ARRAY_AT(&obj_mesh.tris, tri_idx, lvOBJTri);

            for (size_t i = 0; i < 3; i++) {
                normals[j].x = tri.normals[i].x;
                normals[j].y = tri.normals[i].y;
                normals[j].z = tri.normals[i].z;
                j += 1;
            }
        }

        mesh->normals = (lvBuffer){
            .location = 2,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .stride = sizeof(lvVector3),
        };
        if (lvBuffer_init_vertex(&mesh->normals, ctx, sizeof(lvVector3) * mesh->n_vertices, 2) != 0) {
            printf("Buffer creation failed.\n");
            return 1;
        }

        void *normal_buffer_data;
        if (vmaMapMemory(ctx->allocator, mesh->normals._allocation, &normal_buffer_data) != VK_SUCCESS) {
            printf("Memory mapping failed.\n");
            return 1;
        }
        memcpy(normal_buffer_data, normals, sizeof(lvVector3) * mesh->n_vertices);
        vmaUnmapMemory(ctx->allocator, mesh->normals._allocation);

        LV_FREE(normals);
    }

    return 0;
}

void lvMesh_free(lvMesh *mesh, lvContext *ctx) {
    lvBuffer_free(&mesh->normals, ctx);
    lvBuffer_free(&mesh->uvs, ctx);
    lvBuffer_free(&mesh->vertices, ctx);
}