#include "lava/vk/buffer.h"


int lvBuffer_init(
    lvBuffer *buffer,
    lvContext *ctx,
    VkDeviceSize reserved,
    VkBufferUsageFlags usage
) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = reserved,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    if (vmaCreateBuffer(ctx->allocator, &buffer_info, &alloc_info, &buffer->_buffer, &buffer->_allocation, NULL) != VK_SUCCESS) {
        printf("Failed to create buffer.\n");
        return 1;
    }

    return 0;
}

int lvBuffer_init_aligned(
    lvBuffer *buffer,
    lvContext *ctx,
    VkDeviceSize reserved,
    VkBufferUsageFlags usage,
    VkDeviceSize alignment
) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = reserved,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    if (vmaCreateBufferWithAlignment(ctx->allocator, &buffer_info, &alloc_info, alignment, &buffer->_buffer, &buffer->_allocation, NULL) != VK_SUCCESS) {
        printf("Failed to create buffer.\n");
        return 1;
    }

    return 0;
}

int lvBuffer_init_mappable(
    lvBuffer *buffer,
    lvContext *ctx,
    VkDeviceSize reserved,
    VkBufferUsageFlags usage
) {
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = reserved,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    };

    if (vmaCreateBuffer(ctx->allocator, &buffer_info, &alloc_info, &buffer->_buffer, &buffer->_allocation, NULL) != VK_SUCCESS) {
        printf("Failed to create buffer.\n");
        return 1;
    }

    return 0;
}

int lvBuffer_init_vertex(lvBuffer *buffer, lvContext *ctx, VkDeviceSize reserved, uint32_t binding, bool as) {
    buffer->_binding_desc = (VkVertexInputBindingDescription){
        .binding = binding,
        .stride = buffer->stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    buffer->_attr_desc = (VkVertexInputAttributeDescription){
        .binding = binding,
        .location = buffer->location,
        .format = buffer->format,
        .offset = 0
    };

    VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    if (as) {
        usage =
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    }

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = reserved,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    if (vmaCreateBuffer(ctx->allocator, &buffer_info, &alloc_info, &buffer->_buffer, &buffer->_allocation, NULL) != VK_SUCCESS) {
        printf("Failed to create vertex buffer.\n");
        return 1;
    }

    return 0;
}

void lvBuffer_free(lvBuffer *buffer, lvContext *ctx) {
    vmaDestroyBuffer(ctx->allocator, buffer->_buffer, buffer->_allocation);
}