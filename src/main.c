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
#include "vk_mem_alloc.h"
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include "cglm/cglm.h"

#include "lava/lava.h"


void record_cmd_buf(
    lvContext *ctx,
    lvSwapchain *swapchain,
    lvScene *scene,
    lvGraphicsPipeline *graphics_pipeline,
    VkCommandBuffer cmd_buf,
    uint32_t image_idx,
    uint32_t frame_idx_sync,
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
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE // TODO: ONLY FOR DEBUGGING, CHANGE TO DONT_CARE
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

    lvArray vk_buffers = lvArray_new(sizeof(VkBuffer));
    for (size_t model_i = 0; model_i < scene->models.size; model_i++) {
        lvModel *model = LV_ARRAY_PTR_AT(&scene->models, model_i, lvModel);
        lvMesh *mesh = model->meshes.data[0];

        // y * w + x
        // y = frame_i
        // w = n_models
        // x = model_i
        size_t ubo_idx = frame_idx_sync * scene->models.size + model_i;

        vk_buffers.size = 0;
        lvArray_add(&vk_buffers, &(mesh->vertices._buffer));
        lvArray_add(&vk_buffers, &(mesh->uvs._buffer));
        lvArray_add(&vk_buffers, &(mesh->normals._buffer));

        VkDeviceSize offsets[] = {0, 0, 0};
        vkCmdBindVertexBuffers(cmd_buf, 0, 3, (VkBuffer *)vk_buffers.data, offsets);

        //VkDescriptorSet sets_to_bind[1] = {frame_sets[ubo_idx], mat_sets[mesh_i]};

        vkCmdBindDescriptorSets(
            cmd_buf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphics_pipeline->layout,
            2,              // firstSet
            1,              // set count
            &graphics_pipeline->object_sets[ubo_idx],
            0, NULL
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
    mat4 model;
    mat4 view;
    mat4 proj;
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
    const size_t frame_lag = 2;
    if (lvSwapchain_init(&swapchain, &ctx, frame_lag) != 0) {
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
    cmd_bufs.size = frame_lag;
    lvArray_resize(&cmd_bufs);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = ctx.cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = frame_lag
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
    if (mat) {
        printf(
            "- Material:\n"
            "  - Name:      %s\n"
            "  - Roughness: %.2f\n"
            "\n",
            mat->name,
            mat->roughness
        );
    }
    else {
        printf("- Material: None\n");
    }

    lvPrecisionTimer_start(&obj_timer);
    lvOBJ bunny_obj = lvOBJ_load(
        "../assets/models/stanford_bunny_low_poly.obj"
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

    lvCamera camera = {
        .position = {1.4f, 1.4f, 1.4f},
        .dir = {-1.0f, -1.0f, 0.0f},
        .up = {0.0f, 0.0f, 1.0f},
        .fov = 90.0f,
        .near_z = 0.1f,
        .far_z = 100.0f
    };

    lvScene scene = lvScene_new(&camera);

    if (lvScene_add_model(&scene, &ctx, &table_obj, "Table") != 0) {
        lv_fatal("Failed to add model to scene.");
    }

    if (lvScene_add_model(&scene, &ctx, &bunny_obj, "Bunny") != 0) {
        lv_fatal("Failed to add model to scene.");
    }

    printf("Scene is setup! %zu\n", scene.models.size);


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
        LV_RESOURCE_TYPE_UNIFORM,
        LV_RESOURCE_FREQ_OBJECT,
        VK_SHADER_STAGE_VERTEX_BIT,
        sizeof(MVP)
    );

    lvGraphicsPipelineBuilder_define_resource(
        &graphics_builder,
        "Texture",
        LV_RESOURCE_TYPE_SAMPLER,
        LV_RESOURCE_FREQ_MATERIAL,
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

    lvRefArray textures = lvRefArray_new();
    lvRefArray_add(&textures, &texture0);
    lvRefArray_add(&textures, &texture1);


    // //                                 [frame_lag * n_models]
    // VkDescriptorSetLayout frame_layouts[2 * 2];
    // for (uint32_t i = 0; i < frame_lag * n_models; i++) frame_layouts[i] = graphics_pipeline.frame_set_lyt;


    // VkDescriptorSetAllocateInfo frame_set_alloc_info = {
    //     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    //     .pNext = NULL,
    //     .descriptorPool = graphics_pipeline.desc_pool,
    //     .descriptorSetCount = frame_lag * n_models,
    //     .pSetLayouts = frame_layouts
    // };

    // //                                 [n_models]
    // VkDescriptorSetLayout mat_layouts[2];
    // for (uint32_t i = 0; i < 2; i++) mat_layouts[i] = graphics_pipeline.mat_set_lyt;

    // VkDescriptorSetAllocateInfo material_set_alloc_info = {
    //     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    //     .pNext = NULL,
    //     .descriptorPool = graphics_pipeline.desc_pool,
    //     .descriptorSetCount = 2, // n_models
    //     .pSetLayouts = mat_layouts
    // };

    // //                        [frame_lag * n_models]
    // VkDescriptorSet frame_sets[2 * 2];

    // //                          [n_models]
    // VkDescriptorSet material_sets[2];

    // if (vkAllocateDescriptorSets(ctx.device, &frame_set_alloc_info, frame_sets) != VK_SUCCESS) {
    //     printf("Failed to allocate descriptior set for uniforms.\n");
    //     return 1;
    // }

    // if (vkAllocateDescriptorSets(ctx.device, &material_set_alloc_info, material_sets) != VK_SUCCESS) {
    //     printf("Failed to allocate descriptior set for material.\n");
    //     return 1;
    // }

    // for (size_t frame_i = 0; frame_i < frame_lag; frame_i++) {
    //     for (size_t mat_i = 0; mat_i < n_models; mat_i++) {
    //         size_t ubo_idx = frame_i * n_models + mat_i;

    //         VkDescriptorBufferInfo desc_buffer_info = {
    //             .buffer = LV_ARRAY_PTR_AT(&uniforms, ubo_idx, lvBuffer)->_buffer,
    //             .offset = 0,
    //             .range = VK_WHOLE_SIZE
    //         };

    //         VkWriteDescriptorSet desc_write = {
    //             .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //             .pNext = NULL,
    //             .dstSet = frame_sets[ubo_idx],
    //             .dstBinding = 0,
    //             .dstArrayElement = 0,
    //             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //             .descriptorCount = 1,
    //             .pBufferInfo = &desc_buffer_info,
    //             .pImageInfo = NULL,
    //             .pTexelBufferView = NULL
    //         };

    //         vkUpdateDescriptorSets(ctx.device, 1, &desc_write, 0, NULL);
    //     }
    // }

    // for (size_t mat_i = 0; mat_i < n_models; mat_i++) {
    //     lvImage image = *((lvImage *)textures.data[mat_i]);
    //     VkDescriptorImageInfo desc_image_info = {
    //         .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //         .imageView = image.view,
    //         .sampler = image.sampler,
    //     };

    //     VkWriteDescriptorSet desc_write_img = {
    //         .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //         .pNext = NULL,
    //         .dstSet = material_sets[mat_i],
    //         .dstBinding = 0,
    //         .dstArrayElement = 0,
    //         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //         .descriptorCount = 1,
    //         .pBufferInfo = NULL,
    //         .pImageInfo = &desc_image_info,
    //         .pTexelBufferView = NULL
    //     };

    //     vkUpdateDescriptorSets(ctx.device, 1, &desc_write_img, 0, NULL);
    // }





    lvClock clock = lvClock_new();

    lvPrecisionTimer timer;
    lvPrecisionTimer_start(&timer);

    // Frame index used by the synchronization structures
    size_t frame_idx_sync = 0;

    bool is_running = true;
    while (is_running) {
        lvClock_tick(&clock, 60);

        double dt = lvClock_get_delta_time(&clock);
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
        }


        // DRAW FRAME

        VkFence curr_fen = LV_ARRAY_AT(&swapchain.fen_frame, frame_idx_sync, VkFence);
        vkWaitForFences(ctx.device, 1, &curr_fen, VK_TRUE, UINT64_MAX);
        vkResetFences(ctx.device, 1, &curr_fen);

        VkSemaphore sem_img_available = LV_ARRAY_AT(&swapchain.sem_image, frame_idx_sync, VkSemaphore);
        uint32_t image_idx = 0;
        if (vkAcquireNextImageKHR(ctx.device, swapchain.swapchain, UINT64_MAX, sem_img_available, VK_NULL_HANDLE, &image_idx) != VK_SUCCESS) {
            printf("Failed to acquire next image from swapchain, continuing.");
        }

        VkCommandBuffer cmd_buf = LV_ARRAY_AT(&cmd_bufs, frame_idx_sync, VkCommandBuffer);

        vkResetCommandBuffer(cmd_buf, 0);
        record_cmd_buf(
            &ctx,
            &swapchain,
            &scene,
            &graphics_pipeline,
            cmd_buf,
            image_idx,
            frame_idx_sync,
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

            MVP ubo = {
                GLM_MAT4_IDENTITY_INIT,
                GLM_MAT4_IDENTITY_INIT,
                GLM_MAT4_IDENTITY_INIT
            };

            float time = (float)lvPrecisionTimer_stop(&timer);

            float scale = 1.0f;

            if (model_i == 1) {
                scale = 0.2f;
                glm_translate(ubo.model, (vec3){0.0f, 0.0f, 0.85f});
            }

            glm_scale(ubo.model, (vec3){scale, scale, scale});

            glm_rotate(
                ubo.model,
                glm_rad(90.0f),
                (vec3){1.0f, 0.0f, 0.0f}
            );

            glm_rotate(
                ubo.model,
                glm_rad(-time * 90.0f),
                (vec3){0.0f, 1.0f, 0.0f}
            );


            vec3 lookat_target;
            glm_vec3_add(camera.position, camera.dir, lookat_target);
            glm_lookat(camera.position, lookat_target, camera.up, ubo.view);
            glm_perspective(
                camera.fov * 0.0174533f,
                (float)swapchain.extent.width / (float)swapchain.extent.height,
                camera.near_z,
                camera.far_z,
                ubo.proj
            );
            ubo.proj[1][1] *= -1.0f;

            if (lvGraphicsPipeline_set_uniform(
                &graphics_pipeline,
                "MVP",
                &ubo,
                (size_t[2]){frame_idx_sync, model_i}
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

        frame_idx_sync = (frame_idx_sync + 1) % frame_lag;
    }

    // Wait for synchronization to be done before cleanup
    vkDeviceWaitIdle(ctx.device);

    lvRefArray_free(&textures);

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
