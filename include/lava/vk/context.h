#ifndef LAVA_VK_CONTEXT_H
#define LAVA_VK_CONTEXT_H

#include "lava/internal.h"
#include "lava/containers/refarray.h"


typedef struct {
    uint32_t graphics_idx;
    uint32_t present_idx;
} lvQueueFamilies;

typedef struct {
    VkInstance inst;
    VkSurfaceKHR surface;

    VkPhysicalDevice phydevice;
    lvQueueFamilies families;

    VkDevice device;
    VkQueue graphics_q;
    VkQueue present_q;

    VmaAllocator allocator;

    lvRefArray swapchains;

    size_t vertex_bindings;
} lvContext;

int lvContext_init(lvContext *ctx, SDL_Window *window);

void lvContext_free(lvContext *ctx);


#endif // LAVA_VK_CONTEXT_H