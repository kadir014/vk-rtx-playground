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

int lvBuffer_init(
    lvBuffer *buffer,
    lvContext *ctx,
    VkDeviceSize reserved,
    VkBufferUsageFlags usage
);

int lvBuffer_init_aligned(
    lvBuffer *buffer,
    lvContext *ctx,
    VkDeviceSize reserved,
    VkBufferUsageFlags usage,
    VkDeviceSize alignment
);

int lvBuffer_init_mappable(
    lvBuffer *buffer,
    lvContext *ctx,
    VkDeviceSize reserved,
    VkBufferUsageFlags usage
);

int lvBuffer_init_vertex(lvBuffer *buffer, lvContext *ctx, VkDeviceSize reserved, uint32_t binding, bool as);

void lvBuffer_free(lvBuffer *buffer, lvContext *ctx);


#endif // LAVA_VK_BUFFER_H