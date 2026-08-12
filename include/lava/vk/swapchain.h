#ifndef LAVA_VK_SWAPCHAIN_H
#define LAVA_VK_SWAPCHAIN_H

#include "lava/internal.h"
#include "lava/containers/array.h"
#include "lava/vk/context.h"


typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    lvArray formats;
    lvArray present_modes;
} lvSwapChainSupport;

typedef struct {
    VkSwapchainKHR swapchain;

    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode;
    VkExtent2D extent;

    lvArray images;
    lvArray views;

    lvArray sem_image;
    lvArray sem_present;
    lvArray fen_frame;
} lvSwapchain;

/**
 * @brief Create a new swapchain.
 * 
 * The memory of the higher-level `lvSwapchain` is user's responsibility,
 * however the vulkan objects are handled internally and freed with `lvContext_free`.
 * 
 * @param swapchain Pointer to swapchain to initialize.
 * @param ctx Context.
 * @param preferred_extent_width TODO
 * @param preferred_extent_height TODO
 * @return `0` if successful.
 */
int lvSwapchain_init(
    lvSwapchain *swapchain,
    lvContext *ctx,
    uint32_t preferred_extent_width,
    uint32_t preferred_extent_height
);

void lvSwapchain_free(lvSwapchain *swapchain, lvContext *ctx);


#endif // LAVA_VK_SWAPCHAIN_H