#include "lava/vk/image.h"
#include "lava/vk/buffer.h"
#include "lava/vk/swapchain.h"
#include "lava/vk/helpers.h"


int lvImage_init_empty(
    lvImage *image,
    lvContext *ctx,
    uint32_t width,
    uint32_t height,
    SDL_Color color
) {
    image->image = VK_NULL_HANDLE;
    image->alloc = VK_NULL_HANDLE;
    image->view = VK_NULL_HANDLE;
    image->sampler = VK_NULL_HANDLE;
    image->width = 0;
    image->height = 0;

    SDL_Surface *surf = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    if (!surf) {
        printf("Failed to create surface.\n");
        return 1;
    }
    SDL_FillRect(
        surf,
        &(SDL_Rect){0, 0, width, height},
        SDL_MapRGBA(surf->format, color.b, color.g, color.r, color.a)
    );

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
            ctx->allocator,
            &texture_staging_info,
            &texture_staging_alloc_info,
            &texture_staging._buffer,
            &texture_staging._allocation,
            NULL
        ) != VK_SUCCESS
    ) {
        printf("Failed to create staging buffer.");
        return 1;
    }

    void *texture_staging_mapped;
    vmaMapMemory(
        ctx->allocator,
        texture_staging._allocation,
        &texture_staging_mapped
    );
    memcpy(texture_staging_mapped, surf->pixels, surf_size);
    vmaUnmapMemory(ctx->allocator, texture_staging._allocation);

    SDL_FreeSurface(surf);
    
    image->width = surf_width;
    image->height = surf_height;

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

    if (vmaCreateImage(ctx->allocator, &texture_info, &texture_alloc_info, &image->image, &image->alloc, NULL) != VK_SUCCESS) {
        printf("Failed to create texture.");
        return 1;
    }

    lv_transition_image_layout_single_cmd(
        ctx,
        image->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_ACCESS_2_NONE,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT 
    );
    lv_copy_buffer_to_image(ctx, &texture_staging, image);
    lv_transition_image_layout_single_cmd(
        ctx,
        image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT 
    );

    VkImageViewCreateInfo texture_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = image->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };

    if (vkCreateImageView(ctx->device, &texture_view_info, NULL, &image->view) != VK_SUCCESS) {
        printf("Failed to create image view.");
        return 1;
    }

    vmaDestroyBuffer(ctx->allocator, texture_staging._buffer, texture_staging._allocation);

    return 0;
}

int lvImage_init_from_file(lvImage *image, lvContext *ctx, const char *filepath) {
    image->image = VK_NULL_HANDLE;
    image->alloc = VK_NULL_HANDLE;
    image->view = VK_NULL_HANDLE;
    image->sampler = VK_NULL_HANDLE;
    image->width = 0;
    image->height = 0;

    SDL_Surface *surf0 = IMG_Load(filepath);
    if (!surf0) {
        printf("Failed to load surface at '%s'.\n", filepath);
        return 1;
    }

    SDL_Surface *surf = SDL_ConvertSurfaceFormat(surf0, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf0);

    if (!surf) {
        printf("Failed to convert surface: %s\n", SDL_GetError());
        return 1;
    }

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
            ctx->allocator,
            &texture_staging_info,
            &texture_staging_alloc_info,
            &texture_staging._buffer,
            &texture_staging._allocation,
            NULL
        ) != VK_SUCCESS
    ) {
        printf("Failed to create staging buffer.");
        return 1;
    }

    void *texture_staging_mapped;
    vmaMapMemory(
        ctx->allocator,
        texture_staging._allocation,
        &texture_staging_mapped
    );
    memcpy(texture_staging_mapped, surf->pixels, surf_size);
    vmaUnmapMemory(ctx->allocator, texture_staging._allocation);

    SDL_FreeSurface(surf);
    
    image->width = surf_width;
    image->height = surf_height;

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

    if (vmaCreateImage(ctx->allocator, &texture_info, &texture_alloc_info, &image->image, &image->alloc, NULL) != VK_SUCCESS) {
        printf("Failed to create texture.");
        return 1;
    }

    lv_transition_image_layout_single_cmd(
        ctx,
        image->image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_ACCESS_2_NONE,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT 
    );
    lv_copy_buffer_to_image(ctx, &texture_staging, image);
    lv_transition_image_layout_single_cmd(
        ctx,
        image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT 
    );

    VkImageViewCreateInfo texture_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = image->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };

    if (vkCreateImageView(ctx->device, &texture_view_info, NULL, &image->view) != VK_SUCCESS) {
        printf("Failed to create image view.");
        return 1;
    }

    vmaDestroyBuffer(ctx->allocator, texture_staging._buffer, texture_staging._allocation);

    return 0;
}

int lvImage_init_depth(lvImage *image, lvContext *ctx, size_t swapchain_idx) {
    image->image = VK_NULL_HANDLE;
    image->alloc = VK_NULL_HANDLE;
    image->view = VK_NULL_HANDLE;
    image->sampler = VK_NULL_HANDLE;

    image->width = ((lvSwapchain *)ctx->swapchains.data[swapchain_idx])->extent.width;
    image->height = ((lvSwapchain *)ctx->swapchains.data[swapchain_idx])->extent.height;

    VkImageCreateInfo texture_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = image->width,
        .extent.height = image->height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = VK_FORMAT_D32_SFLOAT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = VK_SAMPLE_COUNT_1_BIT
    };

    VmaAllocationCreateInfo depth_texture_alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    if (vmaCreateImage(ctx->allocator, &texture_info, &depth_texture_alloc_info, &image->image, &image->alloc, NULL) != VK_SUCCESS) {
        printf("Failed to create depth texture.");
        return 1;
    }

    VkImageViewCreateInfo depth_texture_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .image = image->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };

    if (vkCreateImageView(ctx->device, &depth_texture_view_info, NULL, &image->view) != VK_SUCCESS) {
        printf("Failed to create depth image view.");
        return 1;
    }

    return 0;
}

void lvImage_free(lvImage *image, lvContext *ctx) {
    vkDestroySampler(ctx->device, image->sampler, NULL);
    vkDestroyImageView(ctx->device, image->view, NULL);
    vmaDestroyImage(ctx->allocator, image->image, image->alloc);
}

int lvImage_build_sampler(
    lvImage *image,
    lvContext *ctx,
    VkFilter mag,
    VkFilter min,
    VkSamplerAddressMode u_address,
    VkSamplerAddressMode v_address,
    VkSamplerAddressMode w_address,
    float max_anisotropy
) {
    VkBool32 use_anisotropy = max_anisotropy > 0.0f;

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = mag,
        .minFilter = min,
        .addressModeU = u_address,
        .addressModeV = v_address,
        .addressModeW = w_address,
        .anisotropyEnable = use_anisotropy,
        .maxAnisotropy = max_anisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f
    };

    if (vkCreateSampler(ctx->device, &sampler_info, NULL, &image->sampler) != VK_SUCCESS) {
        printf("Failed to create image sampler.");
        return 1;
    }

    return 0;
}