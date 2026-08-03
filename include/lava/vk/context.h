#ifndef LAVA_VK_CONTEXT_H
#define LAVA_VK_CONTEXT_H

#include "lava/internal.h"
#include "lava/containers/array.h"
#include "lava/containers/refarray.h"


typedef struct {
    uint32_t graphics_idx;
    uint32_t present_idx;
} lvQueueFamilies;

typedef struct {
    lvRefArray requested_layers;
} lvContextCreation;

/**
 * @brief Context encapsulating the top-level Vulkan objects.
 */
typedef struct {
    lvContextCreation _creation;

    VkInstance inst;
    VkSurfaceKHR surface;

    VkPhysicalDevice phydevice;
    lvQueueFamilies families;

    VkDevice device;
    VkQueue graphics_q;
    VkQueue present_q;

    VkCommandPool cmd_pool;

    VmaAllocator allocator;

    lvRefArray swapchains;

    size_t vertex_bindings;
} lvContext;

static const lvContext lvContext_default = {0};

/**
 * @brief Request a new validation layer.
 * 
 * You must call this before initializing with @ref lvContext_init.
 * 
 * Can be called multiple times to request multiple layers.
 * 
 * Usage:
 * ```
 * lvContext ctx = lvContext_default;
 * 
 * // BEFORE initialization.
 * lvContext_request_validation_layer(&ctx, "VK_LAYER_KHRONOS_validation");
 * lvContext_request_validation_layer(&ctx, "VK_LAYER_RENDERDOC_Capture");
 * lvContext_request_validation_layer(&ctx, ...);
 * 
 * // After requesting layers, we can now initialize.
 * lvContext_init(&ctx);
 * ```
 * 
 * @param ctx Pointer to lvContext.
 * @param layer_name Null-terminated name of the validation layer to request.
 */
void lvContext_request_validation_layer(lvContext *ctx, const char *layer_name);

/**
 * @brief Initialize a context.
 * 
 * @param ctx Pointer to lvContext.
 * @param window SDL_Window.
 * @return Zero if initialization was successful.
 */
int lvContext_init(lvContext *ctx, SDL_Window *window);

/**
 * @brief Free a context.
 * 
 * @param ctx Pointer to lvContext.
 */
void lvContext_free(lvContext *ctx);


#endif // LAVA_VK_CONTEXT_H