#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "lava/lava.h"


void record_cmd_buf(
    lvContext *ctx,
    lvSwapchain *swapchain,
    lvScene *scene,
    lvGraphicsPipeline *graphics_pipeline,
    VkCommandBuffer cmd_buf,
    uint32_t image_idx,
    lvImage depth_texture
) {
    // begin recording
    VkCommandBufferBeginInfo cmd_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL,
    };

    if (vkBeginCommandBuffer(cmd_buf, &cmd_begin_info) != VK_SUCCESS) {
        lv_fatal("Failed to begin recording command buffer.");
    }

    // Transition image layout for rendering
    // Old layout is undefined because previous data is not important, we are going to draw over it anyway
    lv_transition_image_layout(
        cmd_buf,
        LV_ARRAY_AT(&swapchain->images, image_idx, VkImage),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_ACCESS_2_NONE,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    );

    // TODO: READ_BIT ne zaman gerekiyor access için?
    lv_transition_image_layout(
        cmd_buf,
        depth_texture.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT
    );

    // VkClearValue is a union, remember to NOT write both members

    VkClearValue clear_color = {
        .color = {{0.0f, 0.0f, 0.0f, 0.0f}}
    };

    VkClearValue depth_clear_color = {
        .depthStencil = {
            .depth = 1.0f,
            .stencil = 0
        }
    };

    VkRenderingAttachmentInfo attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,
        .imageView = LV_ARRAY_AT(&swapchain->views, image_idx, VkImageView),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .clearValue = clear_color,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };

    VkRenderingAttachmentInfo depth_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,
        .imageView = depth_texture.view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .clearValue = depth_clear_color,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
    };

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderArea.offset = (VkOffset2D){0, 0},
        .renderArea.extent = swapchain->extent,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_info,
        .pDepthAttachment = &depth_attachment_info,
        .pStencilAttachment = NULL
    };

    vkCmdBeginRendering(cmd_buf, &rendering_info);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline->pipeline);

    size_t global_idx = ctx->frame_idx;
    vkCmdBindDescriptorSets(
        cmd_buf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphics_pipeline->layout,
        0,              // firstSet
        1,              // set count
        &graphics_pipeline->global_sets[global_idx],
        0,
        NULL
    );

    lvArray vk_buffers = lvArray_new(sizeof(VkBuffer));
    for (size_t model_i = 0; model_i < scene->models.size; model_i++) {
        lvModel *model = LV_ARRAY_PTR_AT(&scene->models, model_i, lvModel);
        lvMesh *mesh = model->meshes.data[0];
        lvMaterial *material = lvScene_get_material(scene, model->material_name);

        // y * w + x
        // y = frame_i
        // w = n_models
        // x = model_i
        size_t obj_idx = ctx->frame_idx * scene->models.size + model_i;

        vk_buffers.size = 0;
        lvArray_add(&vk_buffers, &(mesh->vertices._buffer));
        lvArray_add(&vk_buffers, &(mesh->uvs._buffer));
        lvArray_add(&vk_buffers, &(mesh->normals._buffer));

        VkDeviceSize offsets[] = {0, 0, 0};
        vkCmdBindVertexBuffers(cmd_buf, 0, 3, (VkBuffer *)vk_buffers.data, offsets);

        if (material) {
            lvGraphicsPipeline_bind_sampler(
                graphics_pipeline,
                cmd_buf,
                material->name
            );
        }

        vkCmdBindDescriptorSets(
            cmd_buf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphics_pipeline->layout,
            2,              // firstSet
            1,              // set count
            &graphics_pipeline->object_sets[obj_idx],
            0,
            NULL
        );
        
        vkCmdDraw(cmd_buf, mesh->n_vertices, 1, 0, 0);
    }
    lvArray_free(&vk_buffers);

    vkCmdEndRendering(cmd_buf);

    // Transition image layout for presentation
    lv_transition_image_layout(
        cmd_buf,
        LV_ARRAY_AT(&swapchain->images, image_idx, VkImage),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
    );

    // stop recording
    if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
        lv_fatal("Failed to record command buffer (vkEndCommandBuffer)");
    }
}


typedef struct {
    lvMatrix4 model;
    lvMatrix4 view;
    lvMatrix4 proj;
} MVP;


int main(int argc, char *argv[]) {
    #ifdef LV_DEBUG
        printf("Built in debug mode.\n\n");
    #endif

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
	    lv_fatal("SDL initialization error: %s", SDL_GetError());
	}

    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
        lv_fatal("SDL_image initialization error: %s", IMG_GetError());
    }

    SDL_Vulkan_LoadLibrary(NULL);

    uint32_t window_width = 1280;
    uint32_t window_height = 720;

    SDL_Window *window = SDL_CreateWindow(
        "Vulkan Playground",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN
    );
    if (!window) {
        lv_fatal("Window creation failed: %s", SDL_GetError());
    }

    lvContext ctx = lvContext_default;

    #ifdef LV_DEBUG
        lvContext_request_validation_layer(&ctx, "VK_LAYER_KHRONOS_validation");
    #endif

    if (lvContext_init(&ctx, window) != 0) {
        lv_fatal("Failed to initialize context.");
    }

    lvSwapchain swapchain;
    if (lvSwapchain_init(&swapchain, &ctx, 2) != 0) {
        lv_fatal("Failed to create swapchain.");
    }


    // COMMAND BUFFERS

    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx.families.graphics_idx
    };
    if (vkCreateCommandPool(ctx.device, &cmd_pool_info, NULL, &ctx.cmd_pool) != VK_SUCCESS) {
        lv_fatal("Failed to create graphics command pool.");
    }

    lvArray cmd_bufs = lvArray_new(sizeof(VkCommandBuffer));
    cmd_bufs.size = ctx.frame_lag;
    lvArray_resize(&cmd_bufs);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = ctx.cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = ctx.frame_lag
    };

    if (vkAllocateCommandBuffers(ctx.device, &alloc_info, (VkCommandBuffer *)cmd_bufs.data) != VK_SUCCESS) {
        lv_fatal("Failed to allocate command buffer.");
    }


    lvPrecisionTimer obj_timer; lvPrecisionTimer_start(&obj_timer);
    lvOBJ table_obj = lvOBJ_load_with_mtl(
        "../assets/models/table.obj",
        "../assets/models/table.mtl"
    );
    double obj_elapsed = lvPrecisionTimer_stop(&obj_timer);
    if (!table_obj.loaded) {
        lv_fatal("Failed to load obj file.");
    }
    printf(
        "Loaded OBJ:\n"
        "- Triangles: %zu\n"
        "- Vertices:  %zu\n"
        "- Elapsed:   %.2f ms\n"
        "\n",
        table_obj.mesh.tris.size,
        table_obj.mesh.tris.size * 3,
        obj_elapsed * 1000.0
    );
    lvOBJMaterialPBR *mat = lvOBJ_get_material(&table_obj, "TableMat");

    lvPrecisionTimer_start(&obj_timer);
    lvOBJ bunny_obj = lvOBJ_load(
        "../assets/models/stanford_bunny_high_poly.obj"
    );
    obj_elapsed = lvPrecisionTimer_stop(&obj_timer);
    if (!bunny_obj.loaded) {
        lv_fatal("Failed to load obj file.");
    }
    printf(
        "Loaded OBJ:\n"
        "- Triangles: %zu\n"
        "- Vertices:  %zu\n"
        "- Elapsed:   %.2f ms\n"
        "\n",
        bunny_obj.mesh.tris.size,
        bunny_obj.mesh.tris.size * 3,
        obj_elapsed * 1000.0
    );

    const char *texture_path = "../assets/textures/table_albedo.png";
    lvImage texture0;
    if (lvImage_init_from_file(&texture0, &ctx, texture_path) != 0) {
        lv_fatal("Failed to load texture.");
    }

    lvImage texture1;
    if (lvImage_init_empty(&texture1, &ctx, 1, 1, (SDL_Color){0, 255, 255, 255}) != 0) {
        lv_fatal("Failed to create texture.");
    }

    VkPhysicalDeviceProperties properties = {0};
    vkGetPhysicalDeviceProperties(ctx.phydevice, &properties);

    if (
        lvImage_build_sampler(
            &texture0,
            &ctx,
            VK_FILTER_LINEAR,
            VK_FILTER_LINEAR,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            properties.limits.maxSamplerAnisotropy
        ) != 0
    ) {
        lv_fatal("Failed to create image sampler 0.");
    }

    if (
        lvImage_build_sampler(
            &texture1,
            &ctx,
            VK_FILTER_NEAREST,
            VK_FILTER_NEAREST,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            VK_SAMPLER_ADDRESS_MODE_REPEAT,
            0.0f
        ) != 0
    ) {
        lv_fatal("Failed to create image sampler 1.");
    }

    lvCamera camera = lvCamera_new_perspective(
        (float)swapchain.extent.width / (float)swapchain.extent.height,
        0.1f, 100.0f,
        55.0f
    );
    camera.position = lv_vector3(0.0f, 1.0f, 3.0f);

    lvScene scene = lvScene_new(&camera);

    if (lvScene_add_material(&scene, &ctx, texture0, "TableMat") != 0) {
        lv_fatal("Failed to create material");
    }

    if (lvScene_add_material(&scene, &ctx, texture1, "FlatMat") != 0) {
        lv_fatal("Failed to create material");
    }

    if (lvScene_add_model(&scene, &ctx, &table_obj, "Table", "TableMat") != 0) {
        lv_fatal("Failed to add model to scene.");
    }

    size_t extent = 1;
    for (size_t y = 0; y < extent; y++) {
        for (size_t x = 0; x < extent; x++) {
            char name[LV_MODEL_NAME_LENGTH];
            sprintf(name, "Bunny%zu", y * extent + x);

            if (lvScene_add_model(&scene, &ctx, &bunny_obj, name, "FlatMat") != 0) {
                lv_fatal("Failed to add model to scene.");
            }

            lvModel *model = lvScene_get_model(&scene, name);
            if (!model) {
                lv_fatal("Model not found: %s", name);
            }

            model->xform.scale.x = 0.1f;
            model->xform.scale.y = 0.1f;
            model->xform.scale.z = 0.1f;
            model->xform.position.x = (float)x * 0.6;
            model->xform.position.y = 0.85f + (float)y * 0.6;
            model->xform.position.z = 0.0f;
        }
    }

    size_t total_tris = 0;
    size_t total_vertices = 0;
    for (size_t i = 0; i < scene.models.size; i++) {
        lvModel *model = LV_ARRAY_PTR_AT(&scene.models, i, lvModel);
        size_t vertices = ((lvMesh *)(model->meshes.data[0]))->n_vertices;
        total_vertices += vertices;
        total_tris += vertices / 3;
    }

    printf(
        "Scene is setup!\n"
        "- Models: %zu\n"
        "- Tris:   %zu\n"
        "- Verts:  %zu\n"
        "\n",
        scene.models.size,
        total_tris,
        total_vertices
    );


    /* RAY TRACING & ACCELERATION STRUCTURES */

    // Scratch buffer needs a weird alignment, fetch it firstly

    VkPhysicalDeviceAccelerationStructurePropertiesKHR as_props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR
    };

    VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &as_props
    };

    vkGetPhysicalDeviceProperties2(ctx.phydevice, &props);

    // Prepare BLAS arrays

    size_t n_blas = scene.models.size;

    lvArray blas_buffers = lvArray_new(sizeof(lvBuffer));
    blas_buffers.size = n_blas;
    lvArray_resize(&blas_buffers);

    lvArray blas_scratch = lvArray_new(sizeof(lvBuffer));
    blas_scratch.size = n_blas;
    lvArray_resize(&blas_scratch);

    lvArray blas_handles = lvArray_new(sizeof(VkAccelerationStructureKHR));
    blas_handles.size = n_blas;
    lvArray_resize(&blas_handles);

    lvArray blas_insts = lvArray_new(sizeof(VkAccelerationStructureInstanceKHR));
    blas_insts.size = n_blas;
    lvArray_resize(&blas_insts);

    for (size_t i = 0; i < n_blas; i++) {
        lvModel *model = LV_ARRAY_PTR_AT(&scene.models, i, lvModel);
        lvMesh *mesh = model->meshes.data[0];

        uint32_t max_primitive_count = mesh->n_vertices / 3;

        VkBufferDeviceAddressInfo vertex_addr_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = mesh->vertices._buffer
        };
        VkDeviceAddress vertex_addr = vkGetBufferDeviceAddress(ctx.device, &vertex_addr_info);

        VkAccelerationStructureGeometryTrianglesDataKHR as_tris = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
            .pNext = NULL,
            .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
            .vertexData.deviceAddress = vertex_addr,
            .vertexStride = sizeof(lvVector3),
            .maxVertex = mesh->n_vertices - 1,
            .indexType = VK_INDEX_TYPE_NONE_KHR,
            .indexData = 0
        };

        VkAccelerationStructureGeometryDataKHR geo_data = {
            .triangles = as_tris
        };

        VkAccelerationStructureGeometryKHR blas_geo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .pNext = NULL,
            .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry = geo_data
        };

        VkAccelerationStructureBuildGeometryInfoKHR blas_geo_build_info = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .pNext = NULL,
            .flags = 0,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .geometryCount = 1,
            .pGeometries = &blas_geo,
            .ppGeometries = NULL,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR
        };

        VkAccelerationStructureBuildSizesInfoKHR blas_build_sizes = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
            .pNext = NULL
        };
        ctx.ext.vkGetAccelerationStructureBuildSizesKHR(
            ctx.device,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &blas_geo_build_info,
            &max_primitive_count,
            &blas_build_sizes
        );

        lvBuffer *buf = LV_ARRAY_PTR_AT(&blas_buffers, i, lvBuffer);
        lvBuffer *scratch = LV_ARRAY_PTR_AT(&blas_scratch, i, lvBuffer);

        if (
            lvBuffer_init_aligned(
                scratch,
                &ctx,
                blas_build_sizes.buildScratchSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                as_props.minAccelerationStructureScratchOffsetAlignment
            ) != 0
        ) {
            lv_fatal("Failed to create BLAS scratch buffer.");
        }

        VkBufferDeviceAddressInfo scratch_addr_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = NULL,
            .buffer = scratch->_buffer
        };
        VkDeviceAddress scratch_addr = vkGetBufferDeviceAddress(ctx.device, &scratch_addr_info);
        blas_geo_build_info.scratchData.deviceAddress = scratch_addr;

        if (
            lvBuffer_init(
                buf,
                &ctx,
                blas_build_sizes.accelerationStructureSize,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
            ) != 0
        ) {
            lv_fatal("Failed to create BLAS buffer.");
        }

        // TODO: VkAccelerationStructureCreateInfo2KHR
        VkAccelerationStructureCreateInfoKHR blas_info = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .pNext = NULL,
            .offset = 0,
            .buffer = buf->_buffer,
            .size = blas_build_sizes.accelerationStructureSize,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        };

        VkAccelerationStructureKHR *blas_handle = LV_ARRAY_PTR_AT(&blas_handles, i, VkAccelerationStructureKHR);

        if (ctx.ext.vkCreateAccelerationStructureKHR(
            ctx.device, &blas_info, NULL, blas_handle
        ) != VK_SUCCESS) {
            lv_fatal("Failed to create BLAS.");
        }

        printf(
            "BLAS for model %zu:\n"
            "- Triangles:    %zu (%zu bytes)\n"
            "- Build size:   %zu\n"
            "- Scratch size: %zu\n"
            "- Min align:    %zu\n"
            "- Buffer:       %p\n"
            "- Scratch:      %p\n"
            "- Handle:       %p\n"
            "\n",
            i,
            mesh->n_vertices / 3, sizeof(lvVector3) * mesh->n_vertices,
            (size_t)blas_build_sizes.accelerationStructureSize,
            (size_t)blas_build_sizes.buildScratchSize,
            (size_t)as_props.minAccelerationStructureScratchOffsetAlignment,
            buf->_buffer,
            scratch->_buffer,
            blas_handle
        );

        blas_geo_build_info.dstAccelerationStructure = *blas_handle;

        // Build info is ready, prepare the ranges

        VkAccelerationStructureBuildRangeInfoKHR blas_range_info = {
            .primitiveCount = max_primitive_count,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
        };

        // build

        VkCommandBuffer cmd_buf = lv_begin_single_time_cmd(&ctx);

        const VkAccelerationStructureBuildRangeInfoKHR *range_info = &blas_range_info;

        ctx.ext.vkCmdBuildAccelerationStructuresKHR(
            cmd_buf,
            1,
            &blas_geo_build_info,
            &range_info
        );

        lv_end_single_time_cmd(&ctx, cmd_buf);

        // Create BLAS instances for TLAS

        VkAccelerationStructureDeviceAddressInfoKHR blas_addr_info = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .pNext = NULL,
            .accelerationStructure = *blas_handle
        };

        VkDeviceAddress blas_addr = ctx.ext.vkGetAccelerationStructureDeviceAddressKHR(ctx.device, &blas_addr_info);

        VkTransformMatrixKHR blas_xform = lvTransform_to_vk_transform_matrix(model->xform);

        VkAccelerationStructureInstanceKHR blas_inst = {
            .accelerationStructureReference = blas_addr,
            .mask = 0xFF,
            .transform = blas_xform
        };

        // TODO: lvArray_set
        memcpy(LV_ARRAY_PTR_AT(&blas_insts, i, VkAccelerationStructureInstanceKHR), &blas_inst, sizeof(VkAccelerationStructureInstanceKHR));
    }

    // PREPARE & BUILD TLAS

    VkDeviceSize inst_size = blas_insts.size * blas_insts.element_size;

    lvBuffer inst_buf;
    if (
        lvBuffer_init_mappable(
            &inst_buf,
            &ctx,
            inst_size,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        ) != 0
    ) {
        lv_fatal("Failed to create instances buffer.");
    }

    void *inst_ptr;
    if (vmaMapMemory(ctx.allocator, inst_buf._allocation, &inst_ptr) != VK_SUCCESS) {
        lv_fatal("Failed to map instance buffer.");
    }
    memcpy(inst_ptr, blas_insts.data, inst_size);
    vmaUnmapMemory(ctx.allocator, inst_buf._allocation);

    VkBufferDeviceAddressInfo inst_buf_addr_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = inst_buf._buffer
    };
    VkDeviceAddress inst_buf_addr = vkGetBufferDeviceAddress(ctx.device, &inst_buf_addr_info);

    VkAccelerationStructureGeometryInstancesDataKHR blas_insts_data = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .pNext = NULL,
        .arrayOfPointers = VK_FALSE,
        .data = inst_buf_addr
    };

    VkAccelerationStructureGeometryDataKHR blas_insts_geo_data = {
        .instances = blas_insts_data
    };

    VkAccelerationStructureGeometryKHR blas_insts_geo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .pNext = NULL,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .geometry = blas_insts_geo_data
        };

    VkAccelerationStructureBuildGeometryInfoKHR tlas_geo_build_info = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .geometryCount = 1,
        .pGeometries = &blas_insts_geo,
        .ppGeometries = NULL,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR
    };

    uint32_t n_tlas_primitives = blas_insts.size;

    VkAccelerationStructureBuildSizesInfoKHR tlas_build_sizes = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        .pNext = NULL
    };
    ctx.ext.vkGetAccelerationStructureBuildSizesKHR(
        ctx.device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &tlas_geo_build_info,
        &n_tlas_primitives,
        &tlas_build_sizes
    );

    lvBuffer tlas_scratch;
    if (
        lvBuffer_init_aligned(
            &tlas_scratch,
            &ctx,
            tlas_build_sizes.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            as_props.minAccelerationStructureScratchOffsetAlignment
        ) != 0
    ) {
        lv_fatal("Failed to create TLAS scratch buffer.");
    }

    VkBufferDeviceAddressInfo tlas_scratch_addr_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = tlas_scratch._buffer
    };
    VkDeviceAddress tlas_scratch_addr = vkGetBufferDeviceAddress(ctx.device, &tlas_scratch_addr_info);
    tlas_geo_build_info.scratchData.deviceAddress = tlas_scratch_addr;

    lvBuffer tlas_buf;
    if (
        lvBuffer_init(
            &tlas_buf,
            &ctx,
            tlas_build_sizes.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        ) != 0
    ) {
        lv_fatal("Failed to create TLAS buffer.");
    }

    VkAccelerationStructureCreateInfoKHR tlas_info = {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext = NULL,
        .offset = 0,
        .buffer = tlas_buf._buffer,
        .size = tlas_build_sizes.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
    };

    VkAccelerationStructureKHR tlas_handle;

    if (ctx.ext.vkCreateAccelerationStructureKHR(
        ctx.device, &tlas_info, NULL, &tlas_handle
    ) != VK_SUCCESS) {
        lv_fatal("Failed to create TLAS.");
    }

    tlas_geo_build_info.dstAccelerationStructure = tlas_handle;

    // Build info is ready, prepare the ranges

    VkAccelerationStructureBuildRangeInfoKHR tlas_range_info = {
        .primitiveCount = n_tlas_primitives,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0
    };

    VkCommandBuffer cmd_buf = lv_begin_single_time_cmd(&ctx);

    const VkAccelerationStructureBuildRangeInfoKHR *range_info = &tlas_range_info;

    ctx.ext.vkCmdBuildAccelerationStructuresKHR(
        cmd_buf,
        1,
        &tlas_geo_build_info,
        &range_info
    );

    lv_end_single_time_cmd(&ctx, cmd_buf);

    scene.tlas = tlas_handle;



    lvGraphicsPipelineBuilder graphics_builder = lvGraphicsPipelineBuilder_new(&ctx, &scene);

    lvGraphicsPipelineBuilder_load_shader(
        &graphics_builder,
        "../shaders/first.vert.spv",
        VK_SHADER_STAGE_VERTEX_BIT
    );

    lvGraphicsPipelineBuilder_load_shader(
        &graphics_builder,
        "../shaders/first.frag.spv",
        VK_SHADER_STAGE_FRAGMENT_BIT
    );

    lvGraphicsPipelineBuilder_define_resource(
        &graphics_builder,
        "MVP",
        lvResourceType_UNIFORM,
        lvResourceScope_OBJECT,
        VK_SHADER_STAGE_VERTEX_BIT,
        sizeof(MVP)
    );

    lvGraphicsPipelineBuilder_define_resource(
        &graphics_builder,
        "Texture",
        lvResourceType_SAMPLER,
        lvResourceScope_MATERIAL,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0
    );

    // TODO: THIS SHOULD BE IMPLICITLY CREATED, NOT BY USER
    lvGraphicsPipelineBuilder_define_resource(
        &graphics_builder,
        "TLAS",
        lvResourceType_ACCELERATION_STRUCTURE,
        lvResourceScope_GLOBAL,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0
    );

    lvGraphicsPipeline graphics_pipeline;
    if (lvGraphicsPipelineBuilder_build(&graphics_builder, &graphics_pipeline) != 0) {
        lv_fatal("Failed to build graphics pipeline.");
    }


    lvImage depth_texture;
    if (lvImage_init_depth(&depth_texture, &ctx, 0) != 0) {
        lv_fatal("Failed to create depth texture.");
    }

    SDL_SetRelativeMouseMode(SDL_TRUE);
    float mouse_sensitivity = 0.1f;


    lvClock clock = lvClock_new();

    lvPrecisionTimer timer;
    lvPrecisionTimer_start(&timer);

    bool is_running = true;
    while (is_running) {
        lvClock_tick(&clock, 165);

        double dt = lvClock_get_delta_time(&clock);
        float dtf = (float)dt;
        double fps = lvClock_get_fps(&clock);

        char title[64];
        sprintf(title, "Vulkan Playground - FPS: %.1f", fps);
        SDL_SetWindowTitle(window, title);

        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                is_running = false;
            }

            else if (
                event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED
            ) {
                window_width = event.window.data1;
                window_height = event.window.data2;
            }

            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    is_running = false;
                }
            }

            else if (event.type == SDL_MOUSEMOTION) {
                camera.yaw += event.motion.xrel * mouse_sensitivity;
                camera.pitch -= event.motion.yrel * mouse_sensitivity;
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);

        if (keys[SDL_SCANCODE_W]) {
            lvCamera_move(&camera, 5.0f * dtf);
        }
        if (keys[SDL_SCANCODE_S]) {
            lvCamera_move(&camera, -5.0f * dtf);
        }
        if (keys[SDL_SCANCODE_E]) {
            camera.position = lvVector3_add(camera.position, lvVector3_mul(camera.up, 5.0f * dtf));
        }
        if (keys[SDL_SCANCODE_Q]) {
            camera.position = lvVector3_add(camera.position, lvVector3_mul(camera.up, -5.0f * dtf));
        }
        if (keys[SDL_SCANCODE_A]) {
            lvCamera_strafe(&camera, -5.0f * dtf);
        }
        if (keys[SDL_SCANCODE_D]) {
            lvCamera_strafe(&camera, 5.0f * dtf);
        }

        lvCamera_update(&camera);


        // DRAW FRAME

        VkFence curr_fen = LV_ARRAY_AT(&swapchain.fen_frame, ctx.frame_idx, VkFence);
        vkWaitForFences(ctx.device, 1, &curr_fen, VK_TRUE, UINT64_MAX);
        vkResetFences(ctx.device, 1, &curr_fen);

        VkSemaphore sem_img_available = LV_ARRAY_AT(&swapchain.sem_image, ctx.frame_idx, VkSemaphore);
        uint32_t image_idx = 0;
        if (vkAcquireNextImageKHR(ctx.device, swapchain.swapchain, UINT64_MAX, sem_img_available, VK_NULL_HANDLE, &image_idx) != VK_SUCCESS) {
            printf("Failed to acquire next image from swapchain, continuing.");
        }

        VkCommandBuffer cmd_buf = LV_ARRAY_AT(&cmd_bufs, ctx.frame_idx, VkCommandBuffer);

        vkResetCommandBuffer(cmd_buf, 0);
        record_cmd_buf(
            &ctx,
            &swapchain,
            &scene,
            &graphics_pipeline,
            cmd_buf,
            image_idx,
            depth_texture
        );

        VkSemaphore sem_render_finished = LV_ARRAY_AT(&swapchain.sem_present, image_idx, VkSemaphore);

        VkPipelineStageFlags wait_stages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &sem_img_available,
            .pWaitDstStageMask = wait_stages,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &sem_render_finished,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buf
        };

        if (vkQueueSubmit(ctx.graphics_q, 1, &submit_info, curr_fen) != VK_SUCCESS) {
            lv_fatal("Failed to submit draw command buffer.");
        }


        // UPDATE UNIFORMS
        
        for (size_t model_i = 0; model_i < scene.models.size; model_i++) {
            lvModel *model = LV_ARRAY_PTR_AT(&scene.models, model_i, lvModel);

            //printf("updating %zu: %s\n", model_i, model_name);

            MVP ubo = {
                lvMatrix4_identity,
                lvMatrix4_identity,
                lvMatrix4_identity
            };

            // glm_mat4_copy(camera.proj_mat, ubo.proj);
            // glm_mat4_copy(camera.view_mat, ubo.view);

            
            // glm_translate(ubo.model, model->xform.position);
            // //glm_euler_xyz(model->xform.rotation, ubo.model);
            // glm_scale(ubo.model, model->xform.scale);


            // float time = (float)lvPrecisionTimer_stop(&timer);

            // glm_rotate(
            //     ubo.model,
            //     glm_rad(-time * 90.0f * model_i),
            //     (vec3){0.0f, 1.0f, 0.0f}
            // );

            ubo.model = lvTransform_to_matrix4(model->xform);
            ubo.proj = camera.proj_mat;
            ubo.view = camera.view_mat;

            if (lvGraphicsPipeline_set_uniform(
                &graphics_pipeline,
                "MVP",
                &ubo,
                (size_t[2]){ctx.frame_idx, model_i}
            ) != 0) {
                lv_fatal("Failed to set uniform.");
            }
        }


        // PRESENT FRAME

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = NULL,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &sem_render_finished,
            .swapchainCount = 1,
            .pSwapchains = &swapchain.swapchain,
            .pImageIndices = &image_idx,
            .pResults = NULL
        };

        if (vkQueuePresentKHR(ctx.present_q, &present_info) != VK_SUCCESS) {
            printf("Failed to present image to swapchain, continuing.");
        }

        ctx.frame_idx = (ctx.frame_idx + 1) % ctx.frame_lag;
    }

    // Wait for synchronization to be done before cleanup
    vkDeviceWaitIdle(ctx.device);

    lvBuffer_free(&tlas_buf, &ctx);
    lvBuffer_free(&tlas_scratch, &ctx);
    lvBuffer_free(&inst_buf, &ctx);
    ctx.ext.vkDestroyAccelerationStructureKHR(ctx.device, tlas_handle, NULL);

    for (size_t i = 0; i < blas_handles.size; i++) {
        ctx.ext.vkDestroyAccelerationStructureKHR(ctx.device, LV_ARRAY_AT(&blas_handles, i, VkAccelerationStructureKHR), NULL);
        lvBuffer_free(LV_ARRAY_PTR_AT(&blas_buffers, i, lvBuffer), &ctx);
        lvBuffer_free(LV_ARRAY_PTR_AT(&blas_scratch, i, lvBuffer), &ctx);
    }
    lvArray_free(&blas_handles);
    lvArray_free(&blas_buffers);
    lvArray_free(&blas_scratch);
    lvArray_free(&blas_insts);

    lvImage_free(&depth_texture, &ctx);

    lvImage_free(&texture0, &ctx);
    lvImage_free(&texture1, &ctx);

    lvOBJ_free(&table_obj);
    lvOBJ_free(&bunny_obj);
    lvScene_free(&scene, &ctx);

    vkDestroyCommandPool(ctx.device, ctx.cmd_pool, NULL);
    lvArray_free(&cmd_bufs);

    lvGraphicsPipeline_free(&graphics_pipeline, &ctx);

    lvContext_free(&ctx);
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    IMG_Quit();
    SDL_Quit();
    
    printf(
        "Exited with errors:\n"
        "- SDL:       '%s'\n"
        "- SDL_image: '%s'\n",
        SDL_GetError(),
        IMG_GetError()
    );

    lv_check_leaks();

    return EXIT_SUCCESS;
}
