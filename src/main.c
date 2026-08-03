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
    lvRefArray *graphics_buffers,
    lvGraphicsPipeline *graphics_pipeline,
    size_t n_verts,
    lvBuffer index_buffer,
    VkCommandBuffer cmd_buf,
    uint32_t image_idx,
    uint32_t frame_idx_sync,
    VkImage depth_texture,
    VkImageView depth_view
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
        depth_texture,
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
        .imageView = depth_view,
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
    for (size_t i = 0; i < graphics_buffers->size; i++) {
        lvBuffer *buf = graphics_buffers->data[i];

        lvArray_add(&vk_buffers, &(buf->_buffer));
    }

    VkDeviceSize offsets[] = {0, 0, 0};
    vkCmdBindVertexBuffers(cmd_buf, 0, graphics_buffers->size, (VkBuffer *)vk_buffers.data, offsets);

    //vkCmdBindIndexBuffer(cmd_buf, index_buffer._buffer, 0, VK_INDEX_TYPE_UINT32);

    lvArray_free(&vk_buffers);

    vkCmdBindDescriptorSets(
        cmd_buf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphics_pipeline->layout,
        0,
        1,
        LV_ARRAY_PTR_AT(&graphics_pipeline->desc_sets, frame_idx_sync, VkDescriptorSet),
        0,
        NULL
    );

    // TODO: Use vertices buffer length here
    vkCmdDraw(cmd_buf, n_verts, 1, 0, 0);

    //vkCmdDrawIndexed(cmd_buf, 6, 1, 0, 0, 0);

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
} UniformBuffer;


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
    lvContext_request_validation_layer(&ctx, "VK_LAYER_KHRONOS_validation");
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
    lvOBJ obj = lvOBJ_load_with_mtl(
        "../assets/models/table.obj",
        "../assets/models/table.mtl"
    );
    double obj_elapsed = lvPrecisionTimer_stop(&obj_timer);
    if (!obj.loaded) {
        lv_fatal("Failed to load obj file.");
    }
    printf(
        "Loaded model:\n"
        "- Triangles: %zu\n"
        "- Vertices:  %zu\n"
        "- Elapsed:   %.2f ms\n"
        "\n",
        obj.mesh.tris.size,
        obj.mesh.tris.size * 3,
        obj_elapsed * 1000.0
    );
    lvOBJMaterialPBR *mat = lvOBJ_get_material(&obj, "TableComb");
    if (mat) {
        printf(
            "  Material:\n"
            "  - Name:      %s\n"
            "  - Roughness: %.2f\n"
            "\n",
            mat->name,
            mat->roughness
        );
    }

    size_t n_verts = obj.mesh.tris.size * 3;

    vec3 *vertices = LV_MALLOC(sizeof(vec3) * n_verts);
    size_t j = 0;
    for (size_t tri_idx = 0; tri_idx < obj.mesh.tris.size; tri_idx++) {
        lvOBJTri tri = LV_ARRAY_AT(&obj.mesh.tris, tri_idx, lvOBJTri);

        for (size_t i = 0; i < 3; i++) {
            vertices[j][0] = tri.vertices[i].x;
            vertices[j][1] = tri.vertices[i].y;
            vertices[j][2] = tri.vertices[i].z;
            j += 1;
        }
    }

    lvBuffer vertex_buffer = {
        .location = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .stride = sizeof(vec3),
    };
    if (lvBuffer_init(&vertex_buffer, &ctx, sizeof(vec3) * n_verts) != 0) {
        lv_fatal("Buffer creation failed.");
    }

    void *vertex_buffer_data;
    if (vmaMapMemory(ctx.allocator, vertex_buffer._allocation, &vertex_buffer_data) != VK_SUCCESS) {
        lv_fatal("Memory mapping failed.");
    }
    memcpy(vertex_buffer_data, vertices, sizeof(vec3) * n_verts);
    vmaUnmapMemory(ctx.allocator, vertex_buffer._allocation);

    LV_FREE(vertices);


    vec2 *uvs = LV_MALLOC(sizeof(vec2) * n_verts);
    j = 0;
    for (size_t tri_idx = 0; tri_idx < obj.mesh.tris.size; tri_idx++) {
        lvOBJTri tri = LV_ARRAY_AT(&obj.mesh.tris, tri_idx, lvOBJTri);

        for (size_t i = 0; i < 3; i++) {
            uvs[j][0] = tri.uvs[i].x;
            uvs[j][1] = tri.uvs[i].y;
            j += 1;
        }
    }

    lvBuffer uv_buffer = {
        .location = 1,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .stride = sizeof(vec2),
    };
    if (lvBuffer_init(&uv_buffer, &ctx, sizeof(vec2) * n_verts) != 0) {
        lv_fatal("Buffer creation failed.");
    }

    void *uv_buffer_data;
    if (vmaMapMemory(ctx.allocator, uv_buffer._allocation, &uv_buffer_data) != VK_SUCCESS) {
        lv_fatal("Memory mapping failed.");
    }
    memcpy(uv_buffer_data, uvs, sizeof(vec2) * n_verts);
    vmaUnmapMemory(ctx.allocator, uv_buffer._allocation);

    LV_FREE(uvs);


    vec3 *normals = LV_MALLOC(sizeof(vec3) * n_verts);
    j = 0;
    for (size_t tri_idx = 0; tri_idx < obj.mesh.tris.size; tri_idx++) {
        lvOBJTri tri = LV_ARRAY_AT(&obj.mesh.tris, tri_idx, lvOBJTri);

        for (size_t i = 0; i < 3; i++) {
            normals[j][0] = tri.normals[i].x;
            normals[j][1] = tri.normals[i].y;
            normals[j][2] = tri.normals[i].z;
            j += 1;
        }
    }

    lvBuffer normal_buffer = {
        .location = 2,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .stride = sizeof(vec3),
    };
    if (lvBuffer_init(&normal_buffer, &ctx, sizeof(vec3) * n_verts) != 0) {
        lv_fatal("Buffer creation failed.");
    }

    void *normal_buffer_data;
    if (vmaMapMemory(ctx.allocator, normal_buffer._allocation, &normal_buffer_data) != VK_SUCCESS) {
        lv_fatal("Memory mapping failed.");
    }
    memcpy(normal_buffer_data, normals, sizeof(vec3) * n_verts);
    vmaUnmapMemory(ctx.allocator, normal_buffer._allocation);

    LV_FREE(normals);


    uint32_t indices[6] = {
        0, 1, 2,
        2, 3, 0
    };

    lvBuffer index_buffer = {0};

    VkBufferCreateInfo index_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = sizeof(uint32_t) * 6,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo index_buffer_alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
    };

    if (vmaCreateBuffer(ctx.allocator, &index_buffer_info, &index_buffer_alloc_info, &index_buffer._buffer, &index_buffer._allocation, NULL) != VK_SUCCESS) {
        lv_fatal("Failed to create index buffer.");
    }

    void *index_buffer_data;
    if (vmaMapMemory(ctx.allocator, index_buffer._allocation, &index_buffer_data) != VK_SUCCESS) {
        lv_fatal("Memory mapping failed.");
    }
    memcpy(index_buffer_data, indices, sizeof(uint32_t) * 6);
    vmaUnmapMemory(ctx.allocator, index_buffer._allocation);


    VkDescriptorSetLayoutBinding ubo_lyt_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL
    };

    lvArray uniforms = lvArray_new(sizeof(lvBuffer));
    lvRefArray uniform_mappings = lvRefArray_new();

    uniforms.size = frame_lag;
    uniform_mappings.size = frame_lag;
    lvArray_resize(&uniforms);
    lvRefArray_resize(&uniform_mappings);

    for (size_t i = 0; i < frame_lag; i++) {
        VkBufferCreateInfo uniform_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .size = sizeof(UniformBuffer),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        // Uniform buffers can be read, written and accessed in random order
        // But maybe choose sequential write for speed
        VmaAllocationCreateInfo uniform_buffer_alloc_info = {
            .usage = VMA_MEMORY_USAGE_AUTO,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT 
        };

        if (
            vmaCreateBuffer(
                ctx.allocator,
                &uniform_buffer_info,
                &uniform_buffer_alloc_info,
                &(LV_ARRAY_PTR_AT(&uniforms, i, lvBuffer)->_buffer),
                &(LV_ARRAY_PTR_AT(&uniforms, i, lvBuffer)->_allocation),
                NULL
            ) != VK_SUCCESS
        ) {
            lv_fatal("Failed to create uniform buffer.");
        }

        vmaMapMemory(
            ctx.allocator,
            LV_ARRAY_PTR_AT(&uniforms, i, lvBuffer)->_allocation,
            uniform_mappings.data + i
        );
    }


    const char *texture_path = "C:/Users/bjkka/Desktop/fantasy-table/textures/TableComb_BaseColor.png";
    lvImage texture;
    lvImage_init_from_file(&texture, &ctx, texture_path);

    VkPhysicalDeviceProperties properties = {0};
    vkGetPhysicalDeviceProperties(ctx.phydevice, &properties);

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f
    };

    VkSampler texture_sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(ctx.device, &sampler_info, NULL, &texture_sampler) != VK_SUCCESS) {
        lv_fatal("Failed to create texture sampler.");
    }

    VkDescriptorSetLayoutBinding sampler_lyt_binding = {
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImmutableSamplers = NULL,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };


    lvImage depth_texture;
    if (lvImage_init_depth(&depth_texture, &ctx, 0) != 0) {
        lv_fatal("Failed to create depth texture.");
    }


    lvRefArray graphics_buffers = lvRefArray_new();
    lvRefArray_add(&graphics_buffers, &vertex_buffer);
    //lvRefArray_add(&graphics_buffers, &color_buffer);
    lvRefArray_add(&graphics_buffers, &uv_buffer);
    lvRefArray_add(&graphics_buffers, &normal_buffer);

    lvArray desc_bindings = lvArray_new(sizeof(VkDescriptorSetLayoutBinding));
    lvArray_add(&desc_bindings, &ubo_lyt_binding);
    lvArray_add(&desc_bindings, &sampler_lyt_binding);

    lvGraphicsPipeline graphics_pipeline;
    if (
        lvGraphicsPipeline_init(
            &graphics_pipeline,
            &ctx,
            0,
            "../shaders/first.vert.spv",
            "../shaders/first.frag.spv",
            &graphics_buffers,
            &uniforms,
            &desc_bindings,
            texture.view,
            texture_sampler
        ) != 0
    ) {
        lv_fatal("Failed to create graphics pipeline.");
    }


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
            &graphics_buffers,
            &graphics_pipeline,
            n_verts,
            index_buffer,
            cmd_buf,
            image_idx,
            frame_idx_sync,
            depth_texture.image,
            depth_texture.view
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

        UniformBuffer ubo = {
            GLM_MAT4_IDENTITY_INIT,
            GLM_MAT4_IDENTITY_INIT,
            GLM_MAT4_IDENTITY_INIT
        };

        float time = (float)lvPrecisionTimer_stop(&timer);

        float scale = 0.8f;
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
        glm_lookat((vec3){2.0f, 2.0f, 2.0f}, (vec3){0.0f, 0.0f, 0.5f}, (vec3){0.0f, 0.0f, 1.0f}, ubo.view);
        glm_perspective(
            45.0f * 0.0174533f,
            (float)swapchain.extent.width / (float)swapchain.extent.height,
            0.1f,
            100.0f,
            ubo.proj
        );
        ubo.proj[1][1] *= -1.0f;

        memcpy(uniform_mappings.data[frame_idx_sync], &ubo, sizeof(ubo));


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

    lvOBJ_free(&obj);

    lvImage_free(&depth_texture, &ctx);

    vkDestroySampler(ctx.device, texture_sampler, NULL);
    lvImage_free(&texture, &ctx);

    for (size_t i = 0; i < frame_lag; i++) {
        vmaUnmapMemory(ctx.allocator, LV_ARRAY_PTR_AT(&uniforms, i, lvBuffer)->_allocation);

        vmaDestroyBuffer(
            ctx.allocator,
            LV_ARRAY_PTR_AT(&uniforms, i, lvBuffer)->_buffer,
            LV_ARRAY_PTR_AT(&uniforms, i, lvBuffer)->_allocation
        );
    }
    lvArray_free(&uniforms);
    lvRefArray_free(&uniform_mappings);

    vmaDestroyBuffer(ctx.allocator, index_buffer._buffer, index_buffer._allocation);
    vmaDestroyBuffer(ctx.allocator, vertex_buffer._buffer, vertex_buffer._allocation);
    vmaDestroyBuffer(ctx.allocator, uv_buffer._buffer, uv_buffer._allocation);
    vmaDestroyBuffer(ctx.allocator, normal_buffer._buffer, normal_buffer._allocation);

    vkDestroyCommandPool(ctx.device, ctx.cmd_pool, NULL);
    lvArray_free(&cmd_bufs);

    lvGraphicsPipeline_free(&graphics_pipeline, &ctx);
    lvRefArray_free(&graphics_buffers);

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
