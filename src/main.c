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


typedef struct {
    vec2 position;
    vec4 color;
} Vertex;


VkShaderModule create_shader_module(VkDevice device, const char *filepath) {
    lvFileContent shader_source = lv_read_file_raw(filepath);
    if (!shader_source.data) {
        lv_fatal("Failed to read shader file: %s", filepath);
    }

    // TODO codeSize zero-terminated length mi istiyor (length+1) yoksa normal length mi?
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = shader_source.length,
        .pCode = (uint32_t *)shader_source.data
    };

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        lv_fatal("Failed to create shader module.");
    }

    LV_FREE(shader_source.data);

    return shader_module;
}


// TODO: GRAPHICS PIPELINE'IN SWAPCHAINE IHTIYACI YOK, sadece extent vs için veriyorum parametre
int create_graphics_pipeline(lvContext *ctx, lvSwapchain *swapchain) {
    // SHADERS

    VkShaderModule vert_module = create_shader_module(ctx->device, "../shaders/first.vert.spv");
    VkShaderModule frag_module = create_shader_module(ctx->device, "../shaders/first.frag.spv");


    // VERTEX BINDING

    VkVertexInputBindingDescription vertex_binding_desc ={
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertex_attr_descs[2];
    vertex_attr_descs[0] = (VkVertexInputAttributeDescription){
        .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, position)
    };
    vertex_attr_descs[1] = (VkVertexInputAttributeDescription){
        .binding = 0,
        .location = 1,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .offset = offsetof(Vertex, color)
    };


    // FIXED PIPELINE

    VkPipelineShaderStageCreateInfo vert_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_module,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo frag_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_module,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo stage_infos[2] = {vert_stage_info, frag_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_desc,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_attr_descs,
    };

    VkPipelineInputAssemblyStateCreateInfo input_ass_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .primitiveRestartEnable = VK_FALSE,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)swapchain->extent.width,
        .height = (float)swapchain->extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .extent = swapchain->extent,
        .offset = (VkOffset2D){0, 0}
    };

    VkPipelineViewportStateCreateInfo viewport_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .scissorCount = 1,
        .pViewports = &viewport,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizer_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
    };

    VkPipelineColorBlendStateCreateInfo color_blending_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
    };
    color_blending_info.blendConstants[0] = 0.0f;
    color_blending_info.blendConstants[1] = 0.0f;
    color_blending_info.blendConstants[2] = 0.0f;
    color_blending_info.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipeline_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    if (vkCreatePipelineLayout(ctx->device, &pipeline_lyt_info, NULL, &ctx->pipeline_lyt) != VK_SUCCESS) {
        printf("Failed to create pipeline layout.");
        return 1;
    }

    
    // GRAPHICS PIPELINE

    VkPipelineRenderingCreateInfo pipeline_rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchain->format.format
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_info,
        .flags = 0,

        // Shader stages
        .stageCount = 2,
        .pStages = stage_infos,

        // Fixed pipeline
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_ass_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterizer_info,
        .pMultisampleState = &multisampling_info,
        .pDepthStencilState = NULL,
        .pColorBlendState = &color_blending_info,
        .pDynamicState = NULL,

        // Layout
        .layout = ctx->pipeline_lyt,

        // Renderpass
        .renderPass = NULL,
        .subpass = 0,

        // For graphics pipeline derivation
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &ctx->graphics_pipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline.");
        return 1;
    }

    vkDestroyShaderModule(ctx->device, frag_module, NULL);
    vkDestroyShaderModule(ctx->device, vert_module, NULL);

    return 0;
}


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
    VkCommandBuffer cmd_buf,
    uint32_t image_idx
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

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphics_pipeline);

    VkDeviceSize offsets[] = {0, };
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &ctx->vertex_buffer, offsets); 

    // TODO: Use vertices buffer length here
    vkCmdDraw(cmd_buf, 3, 1, 0, 0);

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

    if (create_graphics_pipeline(&ctx, &swapchain) != 0) {
        lv_fatal("Failed to create graphics pipeline.");
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


    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = sizeof(Vertex) * 3,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
    };

    // VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    // This flag ^ means it will be written sequentally, NEVER READ

    VmaAllocation allocation;
    if (vmaCreateBuffer(ctx.allocator, &bufferInfo, &allocInfo, &ctx.vertex_buffer, &allocation, NULL) != VK_SUCCESS) {
        lv_fatal("Failed to create vertex buffer.");
    }

    Vertex vertices[3] = {
        {
            .position = { 0.0f, -0.5f },
            .color    = { 1.0f, 0.0f, 0.0f, 1.0f }
        },
        {
            .position = { 0.5f, 0.5f },
            .color    = { 0.0f, 1.0f, 0.0f, 1.0f }
        },
        {
            .position = { -0.5f, 0.5f },
            .color    = { 0.0f, 0.0f, 1.0f, 1.0f }
        }
    };

    void *buffer_data;
    if (vmaMapMemory(ctx.allocator, allocation, &buffer_data) != VK_SUCCESS) {
        lv_fatal("Memory mapping failed.");
    }
    memcpy(buffer_data, vertices, sizeof(Vertex) * 3);
    vmaUnmapMemory(ctx.allocator, allocation);


    lvClock clock = lvClock_new();

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
            cmd_buf,
            image_idx
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

    vmaDestroyBuffer(ctx.allocator, ctx.vertex_buffer, allocation);

    vkDestroyCommandPool(ctx.device, cmd_pool, NULL);
    lvArray_free(&cmd_bufs);

    vkDestroyPipeline(ctx.device, ctx.graphics_pipeline, NULL);

    vkDestroyPipelineLayout(ctx.device, ctx.pipeline_lyt, NULL);

    lvContext_free(&ctx);
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    
    printf("Exited with SDL_GetError: '%s'\n", SDL_GetError());

    return EXIT_SUCCESS;
}
