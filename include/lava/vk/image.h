#ifndef LAVA_VK_IMAGE_H
#define LAVA_VK_IMAGE_H

#include "lava/internal.h"
#include "lava/vk/context.h"


typedef struct {
    VkImage image;
    VmaAllocation alloc;
    VkImageView view;

    uint32_t width;
    uint32_t height;
} lvImage;

int lvImage_init_empty(lvImage *image, lvContext *ctx, uint32_t width, uint32_t height);

int lvImage_init_from_file(lvImage *image, lvContext *ctx, const char *filepath);

int lvImage_init_depth(lvImage *image, lvContext *ctx, size_t swapchain_idx);

void lvImage_free(lvImage *image, lvContext *ctx);


#endif // LAVA_VK_IMAGE_H