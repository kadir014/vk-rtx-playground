#ifndef LAVA_VK_BUFFER_H
#define LAVA_VK_BUFFER_H

#include "lava/internal.h"
#include "lava/vk/context.h"


typedef struct {
    uint32_t location;
    VkFormat format;
    uint32_t stride;

    VkBuffer _buffer;
    VmaAllocation _allocation;
    VkVertexInputBindingDescription _binding_desc;
    VkVertexInputAttributeDescription _attr_desc;
} lvBuffer;

int lvBuffer_init(lvBuffer *buffer, lvContext *ctx, VkDeviceSize reserved);

int lvBuffer_init_vertex(lvBuffer *buffer, lvContext *ctx, VkDeviceSize reserved, uint32_t binding);

void lvBuffer_free(lvBuffer *buffer, lvContext *ctx);


#endif // LAVA_VK_BUFFER_H