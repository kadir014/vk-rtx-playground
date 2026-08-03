#include "lava/vk/helpers.h"


VkCommandBuffer lv_begin_single_time_cmd(lvContext *ctx) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = ctx->cmd_pool,
        .commandBufferCount = 1
    };

    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(ctx->device, &alloc_info, &cmd_buf);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(cmd_buf, &begin_info);

    return cmd_buf;
}

void lv_end_single_time_cmd(lvContext *ctx, VkCommandBuffer cmd_buf) {
    // TODO VK_RESULT CHECKS
    vkEndCommandBuffer(cmd_buf);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buf
    };

    vkQueueSubmit(ctx->graphics_q, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx->graphics_q);

    // Command buffers are usually freed with memory pool, but this buffer
    // is created every time a "single time" buffer is requested, so better
    // cleanup ourselves.
    vkFreeCommandBuffers(ctx->device, ctx->cmd_pool, 1, &cmd_buf);
}

void lv_transition_image_layout(
    VkCommandBuffer cmd_buf,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkImageAspectFlags aspect_mask,
    VkAccessFlags2 src_access_mask,
    VkAccessFlags2 dst_access_mask,
    VkPipelineStageFlags2 src_stage_mask,
    VkPipelineStageFlags2 dst_stage_mask
) {
    // Uses Synchronization2 features to transition layouts

    // From https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/03_Drawing/01_Command_buffers.html#_image_layout_transitions

    const VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = NULL,
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    const VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

	vkCmdPipelineBarrier2(cmd_buf, &dependency_info);
}

void lv_transition_image_layout_single_cmd(
    lvContext *ctx,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkImageAspectFlags aspect_mask,
    VkAccessFlags2 src_access_mask,
    VkAccessFlags2 dst_access_mask,
    VkPipelineStageFlags2 src_stage_mask,
    VkPipelineStageFlags2 dst_stage_mask
) {
    VkCommandBuffer cmd_buf = lv_begin_single_time_cmd(ctx);

    lv_transition_image_layout(
        cmd_buf,
        image,
        old_layout,
        new_layout,
        aspect_mask,
        src_access_mask,
        dst_access_mask,
        src_stage_mask,
        dst_stage_mask
    );

    lv_end_single_time_cmd(ctx, cmd_buf);
}

void lv_copy_buffer_to_image(
    lvContext *ctx,
    lvBuffer *src_buffer,
    lvImage *dst_image
) {
    VkCommandBuffer cmd_buf = lv_begin_single_time_cmd(ctx);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageOffset = {0, 0, 0},
        .imageExtent = {dst_image->width, dst_image->height, 1},
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1
    };

    vkCmdCopyBufferToImage(
        cmd_buf,
        src_buffer->_buffer,
        dst_image->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    lv_end_single_time_cmd(ctx, cmd_buf);
}