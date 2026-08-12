#include "lava/world/mesh.h"
#include "lava/containers/array.h"
#include "lava/math/vector.h"
#include "lava/vk/helpers.h"


int lvMesh_init(lvMesh *mesh, lvContext *ctx, lvOBJMesh obj_mesh) {
    if (obj_mesh.tris.size == 0 || !lvArray_valid(&obj_mesh.tris)) {
        return 1;
    }

    mesh->n_vertices = obj_mesh.tris.size * 3;

    {
        size_t size = sizeof(lvVector3) * mesh->n_vertices;
        mesh->vertices_arr = LV_MALLOC(size);
        if (!mesh->vertices_arr ) {
            printf("Failed to allocate.\n");
            return 1;
        }

        size_t j = 0;
        for (size_t tri_idx = 0; tri_idx < obj_mesh.tris.size; tri_idx++) {
            lvOBJTri tri = LV_ARRAY_AT(&obj_mesh.tris, tri_idx, lvOBJTri);

            for (size_t i = 0; i < 3; i++) {
                mesh->vertices_arr [j].x = tri.vertices[i].x;
                mesh->vertices_arr [j].y = tri.vertices[i].y;
                mesh->vertices_arr [j].z = tri.vertices[i].z;
                j += 1;
            }
        }

        mesh->vertices = (lvBuffer){
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .stride = sizeof(lvVector3),
        };
        if (lvBuffer_init_vertex(&mesh->vertices, ctx, size, 0, true) != 0) {
            printf("Buffer creation failed.\n");
            return 1;
        }



        VkBufferCreateInfo staging_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        VmaAllocationCreateInfo staging_alloc_info = {
            .usage = VMA_MEMORY_USAGE_AUTO,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        };

        lvBuffer staging;
        if (
            vmaCreateBuffer(
                ctx->allocator,
                &staging_info,
                &staging_alloc_info,
                &staging._buffer,
                &staging._allocation,
                NULL
            ) != VK_SUCCESS
        ) {
            printf("Failed to create staging buffer.");
            return 1;
        }

        void *staging_mapped;
        vmaMapMemory(
            ctx->allocator,
            staging._allocation,
            &staging_mapped
        );
        memcpy(staging_mapped, mesh->vertices_arr , size);
        vmaUnmapMemory(ctx->allocator, staging._allocation);

        lv_copy_buffer_to_buffer(
            ctx,
            &staging,
            &mesh->vertices,
            size
        );
        vmaDestroyBuffer(ctx->allocator, staging._buffer, staging._allocation);



        //LV_FREE(vertices);
    }

    {
        size_t size = sizeof(lvVector2) * mesh->n_vertices;
        lvVector2 *uvs = LV_MALLOC(size);
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
        if (lvBuffer_init_vertex(&mesh->uvs, ctx, size, 1, false) != 0) {
            printf("Buffer creation failed.\n");
            return 1;
        }

        

        VkBufferCreateInfo staging_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        VmaAllocationCreateInfo staging_alloc_info = {
            .usage = VMA_MEMORY_USAGE_AUTO,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        };

        lvBuffer staging;
        if (
            vmaCreateBuffer(
                ctx->allocator,
                &staging_info,
                &staging_alloc_info,
                &staging._buffer,
                &staging._allocation,
                NULL
            ) != VK_SUCCESS
        ) {
            printf("Failed to create staging buffer.");
            return 1;
        }

        void *staging_mapped;
        vmaMapMemory(
            ctx->allocator,
            staging._allocation,
            &staging_mapped
        );
        memcpy(staging_mapped, uvs, size);
        vmaUnmapMemory(ctx->allocator, staging._allocation);

        lv_copy_buffer_to_buffer(
            ctx,
            &staging,
            &mesh->uvs,
            size
        );
        vmaDestroyBuffer(ctx->allocator, staging._buffer, staging._allocation);



        LV_FREE(uvs);
    }

    {
        size_t size = sizeof(lvVector3) * mesh->n_vertices;
        lvVector3 *normals = LV_MALLOC(size);
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
        if (lvBuffer_init_vertex(&mesh->normals, ctx, size, 2, false) != 0) {
            printf("Buffer creation failed.\n");
            return 1;
        }

        

        VkBufferCreateInfo staging_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        VmaAllocationCreateInfo staging_alloc_info = {
            .usage = VMA_MEMORY_USAGE_AUTO,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
        };

        lvBuffer staging;
        if (
            vmaCreateBuffer(
                ctx->allocator,
                &staging_info,
                &staging_alloc_info,
                &staging._buffer,
                &staging._allocation,
                NULL
            ) != VK_SUCCESS
        ) {
            printf("Failed to create staging buffer.");
            return 1;
        }

        void *staging_mapped;
        vmaMapMemory(
            ctx->allocator,
            staging._allocation,
            &staging_mapped
        );
        memcpy(staging_mapped, normals, size);
        vmaUnmapMemory(ctx->allocator, staging._allocation);

        lv_copy_buffer_to_buffer(
            ctx,
            &staging,
            &mesh->normals,
            size
        );
        vmaDestroyBuffer(ctx->allocator, staging._buffer, staging._allocation);

        

        LV_FREE(normals);
    }

    return 0;
}

void lvMesh_free(lvMesh *mesh, lvContext *ctx) {
    LV_FREE(mesh->vertices_arr);
    lvBuffer_free(&mesh->normals, ctx);
    lvBuffer_free(&mesh->uvs, ctx);
    lvBuffer_free(&mesh->vertices, ctx);
}