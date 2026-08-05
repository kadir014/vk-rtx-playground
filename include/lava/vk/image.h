#ifndef LAVA_VK_IMAGE_H
#define LAVA_VK_IMAGE_H

#include "lava/internal.h"
#include "lava/vk/context.h"


typedef struct {
    VkImage image;
    VmaAllocation alloc;
    VkImageView view;
    VkSampler sampler;

    uint32_t width;
    uint32_t height;
} lvImage;

int lvImage_init_empty(
    lvImage *image,
    lvContext *ctx,
    uint32_t width,
    uint32_t height,
    SDL_Color color
);

int lvImage_init_from_file(lvImage *image, lvContext *ctx, const char *filepath);

int lvImage_init_depth(lvImage *image, lvContext *ctx, size_t swapchain_idx);

void lvImage_free(lvImage *image, lvContext *ctx);

/**
 * @brief Build a sampler for the image.
 * 
 * @param image Pointer to lvImage.
 * @param mag Magnification filter.
 * @param min Minification filter.
 * @param u_address Wrapping operation for the U axis out of bounds.
 * @param v_address Wrapping operation for the V axis out of bounds.
 * @param w_address Wrapping operation for the W axis out of bounds.
 * @param max_anisotropy Maximum anisotropy degree.
 *                       If 0.0, anisotropic filtering is disabled.
 * @return `0` if successful.
 */
int lvImage_build_sampler(
    lvImage *image,
    lvContext *ctx,
    VkFilter mag,
    VkFilter min,
    VkSamplerAddressMode u_address,
    VkSamplerAddressMode v_address,
    VkSamplerAddressMode w_address,
    float max_anisotropy
);


#endif // LAVA_VK_IMAGE_H