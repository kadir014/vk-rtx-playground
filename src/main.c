#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "cglm/cglm.h"

#include "lava/lava.h"


void transition_image_layout(
    VkCommandBuffer cmd_buf,
    uint32_t image_idx,
    VkImage *swapchain_images,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkAccessFlags2 src_access_mask,
    VkAccessFlags2 dst_access_mask,
    VkPipelineStageFlags2 src_stage_mask,
    VkPipelineStageFlags2 dst_stage_mask
) {
    // From https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/03_Drawing/01_Command_buffers.html#_image_layout_transitions

    const VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_images[image_idx],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    const VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        //.dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

	vkCmdPipelineBarrier2(cmd_buf, &dependency_info);
}


void record_cmd_buf(
    lvContext *ctx,
    lvSwapchain *swapchain,
    lvRefArray *graphics_buffers,
    lvGraphicsPipeline *graphics_pipeline,
    lvBuffer index_buffer,
    VkCommandBuffer cmd_buf,
    uint32_t image_idx,
    uint32_t frame_idx_sync
) {
    // begin recording
    VkCommandBufferBeginInfo cmd_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };

    if (vkBeginCommandBuffer(cmd_buf, &cmd_begin_info) != VK_SUCCESS) {
        lv_fatal("Failed to begin recording command buffer.");
    }

    // Transition image layout for rendering
    transition_image_layout(
        cmd_buf,
        image_idx,
        (VkImage *)(swapchain->images.data),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_2_NONE,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    );

    VkClearValue clear_color = {
        .color = (VkClearColorValue){1.0f, 0.0f, 0.0f, 1.0f},
        .depthStencil = 0
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

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderArea.offset = (VkOffset2D){0, 0},
        .renderArea.extent = swapchain->extent,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_info
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

    vkCmdBindIndexBuffer(cmd_buf, index_buffer._buffer, 0, VK_INDEX_TYPE_UINT32);

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
    //vkCmdDraw(cmd_buf, 3, 1, 0, 0);

    vkCmdDrawIndexed(cmd_buf, 6, 1, 0, 0, 0);

    vkCmdEndRendering(cmd_buf);

    // Transition image layout for presentation
    transition_image_layout(
        cmd_buf,
        image_idx,
        (VkImage *)(swapchain->images.data),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
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
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
	    lv_fatal("SDL initialization error: %s", SDL_GetError());
        exit(EXIT_FAILURE);
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

    lvContext ctx;
    if (lvContext_init(&ctx, window) != 0) {
        lv_fatal("Failed to initialize context.");
    }

    lvSwapchain swapchain;
    const size_t frame_lag = 2;
    if (lvSwapchain_init(&swapchain, &ctx, frame_lag) != 0) {
        lv_fatal("Failed to create swapchain.");
    }


    // COMMAND BUFFERS

    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx.families.graphics_idx
    };
    if (vkCreateCommandPool(ctx.device, &cmd_pool_info, NULL, &cmd_pool) != VK_SUCCESS) {
        lv_fatal("Failed to create graphics command pool.");
    }

    lvArray cmd_bufs = lvArray_new(sizeof(VkCommandBuffer));
    cmd_bufs.size = frame_lag;
    lvArray_resize(&cmd_bufs);

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = frame_lag
    };

    if (vkAllocateCommandBuffers(ctx.device, &alloc_info, (VkCommandBuffer *)cmd_bufs.data) != VK_SUCCESS) {
        lv_fatal("Failed to allocate command buffer.");
    }


    vec2 vertices[4] = {
        {-0.5f, -0.5f},
        { 0.5f, -0.5f},
        { 0.5f,  0.5f},
        {-0.5f,  0.5f}
    };

    lvBuffer vertex_buffer = {
        .location = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .stride = sizeof(vec2),
    };
    if (lvBuffer_init(&vertex_buffer, &ctx, sizeof(vec2) * 4) != 0) {
        lv_fatal("Buffer creation failed.");
    }

    void *vertex_buffer_data;
    if (vmaMapMemory(ctx.allocator, vertex_buffer._allocation, &vertex_buffer_data) != VK_SUCCESS) {
        lv_fatal("Memory mapping failed.");
    }
    memcpy(vertex_buffer_data, vertices, sizeof(vec2) * 4);
    vmaUnmapMemory(ctx.allocator, vertex_buffer._allocation);


    vec4 colors[4] = {
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f }
    };

    lvBuffer color_buffer = {
        .location = 1,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .stride = sizeof(vec4),
    };
    if (lvBuffer_init(&color_buffer, &ctx, sizeof(vec4) * 4) != 0) {
        lv_fatal("Buffer creation failed.");
    }

    void *color_buffer_data;
    if (vmaMapMemory(ctx.allocator, color_buffer._allocation, &color_buffer_data) != VK_SUCCESS) {
        lv_fatal("Memory mapping failed.");
    }
    memcpy(color_buffer_data, colors, sizeof(vec4) * 4);
    vmaUnmapMemory(ctx.allocator, color_buffer._allocation);


    vec2 uvs[4] = {
        { 0.0f, 0.0f},
        { 1.0f, 0.0f},
        { 1.0f, 1.0f},
        { 0.0f, 1.0f}
    };

    lvBuffer uv_buffer = {
        .location = 2,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .stride = sizeof(vec2),
    };
    if (lvBuffer_init(&uv_buffer, &ctx, sizeof(vec2) * 4) != 0) {
        lv_fatal("Buffer creation failed.");
    }

    void *uv_buffer_data;
    if (vmaMapMemory(ctx.allocator, uv_buffer._allocation, &uv_buffer_data) != VK_SUCCESS) {
        lv_fatal("Memory mapping failed.");
    }
    memcpy(uv_buffer_data, uvs, sizeof(vec2) * 4);
    vmaUnmapMemory(ctx.allocator, uv_buffer._allocation);


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



    lvRefArray graphics_buffers = lvRefArray_new();
    lvRefArray_add(&graphics_buffers, &vertex_buffer);
    lvRefArray_add(&graphics_buffers, &color_buffer);
    lvRefArray_add(&graphics_buffers, &uv_buffer);

    lvArray desc_bindings = lvArray_new(sizeof(VkDescriptorSetLayoutBinding));
    lvArray_add(&desc_bindings, &ubo_lyt_binding);

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
            &desc_bindings
        ) != 0
    ) {
        lv_fatal("Failed to create graphics pipeline.");
    }

    // LEAK:
    //lvRefArray_free(&graphics_buffers);


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
            index_buffer,
            cmd_buf,
            image_idx,
            frame_idx_sync
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

        glm_rotate(
            ubo.model,
            glm_rad(time * 90.0f),
            (vec3){0.0f, 0.0f, 1.0f}
        );
        glm_lookat((vec3){2.0f, 2.0f, 2.0f}, (vec3){0.0f, 0.0f, 0.0f}, (vec3){0.0f, 0.0f, 1.0f}, ubo.view);
        glm_perspective(
            45.0f * 0.0174533f,
            (float)swapchain.extent.width / (float)swapchain.extent.height,
            0.1f,
            1000.0f,
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
    vmaDestroyBuffer(ctx.allocator, color_buffer._buffer, color_buffer._allocation);
    vmaDestroyBuffer(ctx.allocator, uv_buffer._buffer, uv_buffer._allocation);

    vkDestroyCommandPool(ctx.device, cmd_pool, NULL);
    lvArray_free(&cmd_bufs);

    lvGraphicsPipeline_free(&graphics_pipeline, &ctx);

    lvContext_free(&ctx);
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    
    printf("Exited with SDL_GetError: '%s'\n", SDL_GetError());

    return EXIT_SUCCESS;
}
