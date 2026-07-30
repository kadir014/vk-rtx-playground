#include "lava/vk/buffer.h"


int lvBuffer_init(lvBuffer *buffer, lvContext *ctx, VkDeviceSize reserved) {
    buffer->_binding_desc = (VkVertexInputBindingDescription){
        .binding = ctx->vertex_bindings,
        .stride = buffer->stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    buffer->_attr_desc = (VkVertexInputAttributeDescription){
        .binding = ctx->vertex_bindings,
        .location = buffer->location,
        .format = buffer->format,
        .offset = 0
    };

    // TODO: Check against GPU's max vertex bindings count
    ctx->vertex_bindings++;

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = reserved,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT 
    };

    // VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    // This flag ^ means it will be written sequentally, NEVER READ

    if (vmaCreateBuffer(ctx->allocator, &buffer_info, &alloc_info, &buffer->_buffer, &buffer->_allocation, NULL) != VK_SUCCESS) {
        printf("Failed to create vertex buffer.\n");
        return 1;
    }

    return 0;
}