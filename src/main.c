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
#include "cglm/cglm.h"

#include "lava/lava.h"


VkCommandBuffer begin_single_time_cmd(lvContext *ctx) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = ctx->cmd_pool,
        .commandBufferCount = 1
    };

    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(ctx->device, &alloc_info, &cmd_buf);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(cmd_buf, &begin_info);

    return cmd_buf;
}

void end_single_time_cmd(lvContext *ctx, VkCommandBuffer cmd_buf) {
    // TODO VK_RESULT CHECKS
    vkEndCommandBuffer(cmd_buf);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buf
    };

    vkQueueSubmit(ctx->graphics_q, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx->graphics_q);

    // Command buffers are usually freed with memory pool, but this buffer
    // is created every time a "single time" buffer is requested, so better
    // cleanup ourselves.
    vkFreeCommandBuffers(ctx->device, ctx->cmd_pool, 1, &cmd_buf);
}


void transition_image_layout(
    VkCommandBuffer cmd_buf,
    VkImage image,
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
        .image = image,
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
        .pImageMemoryBarriers = &barrier,
    };

	vkCmdPipelineBarrier2(cmd_buf, &dependency_info);
}

void transition_image_layout_single_cmd(
    lvContext *ctx,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkAccessFlags2 src_access_mask,
    VkAccessFlags2 dst_access_mask,
    VkPipelineStageFlags2 src_stage_mask,
    VkPipelineStageFlags2 dst_stage_mask
) {
    VkCommandBuffer cmd_buf = begin_single_time_cmd(ctx);

    transition_image_layout(
        cmd_buf,
        image,
        old_layout,
        new_layout,
        src_access_mask,
        dst_access_mask,
        src_stage_mask,
        dst_stage_mask
    );

    end_single_time_cmd(ctx, cmd_buf);
}


void record_cmd_buf(
    lvContext *ctx,
    lvSwapchain *swapchain,
    lvRefArray *graphics_buffers,
    lvGraphicsPipeline *graphics_pipeline,
    size_t n_verts,
    lvBuffer index_buffer,
    VkCommandBuffer cmd_buf,
    uint32_t image_idx,
    uint32_t frame_idx_sync
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
    transition_image_layout(
        cmd_buf,
        LV_ARRAY_AT(&swapchain->images, image_idx, VkImage),
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
    transition_image_layout(
        cmd_buf,
        LV_ARRAY_AT(&swapchain->images, image_idx, VkImage),
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

// Assumes image is transitioned to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
void copy_buffer_to_image(lvContext *ctx, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer cmd_buf = begin_single_time_cmd(ctx);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1},
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1
    };

    vkCmdCopyBufferToImage(
        cmd_buf,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    end_single_time_cmd(ctx, cmd_buf);
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


    lvOBJ obj = lvOBJ_load("../assets/models/plane.obj");
    if (!obj.loaded) {
        lv_fatal("Failed to load obj file.");
    }
    printf("Loaded obj with %zu triangles.\n", obj.mesh.tris.size);

    size_t n_verts = obj.mesh.tris.size * 3;


    // n_verts = 3;
    // vec3 vertices[3] = {
    //     {-0.5f, -0.5f, 0.0f},
    //     { 0.5f, -0.5f, 0.0f},
    //     { 0.5f,  0.5f, 0.0f}
    // };

    vec3 *vertices = malloc(sizeof(vec3) * n_verts);
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

    free(vertices);


    vec2 *uvs = malloc(sizeof(vec2) * n_verts);
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

    free(uvs);


    vec3 *normals = malloc(sizeof(vec3) * n_verts);
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

    free(normals);


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


    SDL_Surface *surf = IMG_Load("../assets/textures/statue_2k.png");
    surf = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    // TODO: Is the previous surface leaked? or freed implicitly?
    size_t surf_channels = surf->format->BytesPerPixel;
    uint32_t surf_width = surf->w;
    uint32_t surf_height = surf->h;
    size_t surf_size = surf_width * surf_height * surf_channels;
    lvBuffer texture_staging;

    VkBufferCreateInfo texture_staging_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = surf_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo texture_staging_alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    };

    if (
        vmaCreateBuffer(
            ctx.allocator,
            &texture_staging_info,
            &texture_staging_alloc_info,
            &texture_staging._buffer,
            &texture_staging._allocation,
            NULL
        ) != VK_SUCCESS
    ) {
        lv_fatal("Failed to create uniform buffer.");
    }

    void *texture_staging_mapped;
    vmaMapMemory(
        ctx.allocator,
        texture_staging._allocation,
        &texture_staging_mapped
    );
    memcpy(texture_staging_mapped, surf->pixels, surf_size);
    vmaUnmapMemory(ctx.allocator, texture_staging._allocation);

    SDL_FreeSurface(surf);

    VkImage texture;
    VmaAllocation texture_alloc;

    VkImageCreateInfo texture_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = surf_width,
        .extent.height = surf_height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = VK_FORMAT_R8G8B8A8_SRGB, // TODO: NOT ALL TYPES ARE SUPPORTED, use vkGetPhysicalDeviceImageFormatProperties
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = VK_SAMPLE_COUNT_1_BIT
    };

    VmaAllocationCreateInfo texture_alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    if (vmaCreateImage(ctx.allocator, &texture_info, &texture_alloc_info, &texture, &texture_alloc, NULL) != VK_SUCCESS) {
        lv_fatal("Failed to create texture.");
    }

    transition_image_layout_single_cmd(
        &ctx,
        texture,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_ACCESS_2_NONE,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT 
    );
    copy_buffer_to_image(&ctx, texture_staging._buffer, texture, surf_width, surf_height);
    transition_image_layout_single_cmd(
        &ctx,
        texture,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT 
    );

    VkImageViewCreateInfo texture_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = texture,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };

    VkImageView texture_view = VK_NULL_HANDLE;
    if (vkCreateImageView(ctx.device, &texture_view_info, NULL, &texture_view) != VK_SUCCESS) {
        lv_fatal("Failed to create image view.");
    }

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
            texture_view,
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

        glm_scale(ubo.model, (vec3){0.8f, 0.8f, 0.8f});

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

    vkDestroySampler(ctx.device, texture_sampler, NULL);
    vkDestroyImageView(ctx.device, texture_view, NULL);
    vmaDestroyImage(ctx.allocator, texture, texture_alloc);
    vmaDestroyBuffer(ctx.allocator, texture_staging._buffer, texture_staging._allocation);

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
