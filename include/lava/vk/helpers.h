#ifndef LAVA_VK_HELPERS_H
#define LAVA_VK_HELPERS_H

#include "lava/internal.h"
#include "lava/vk/context.h"
#include "lava/vk/buffer.h"
#include "lava/vk/image.h"


VkCommandBuffer lv_begin_single_time_cmd(lvContext *ctx);

void lv_end_single_time_cmd(lvContext *ctx, VkCommandBuffer cmd_buf);

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
);

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
);

/**
 * @brief Copy buffer to image.
 * 
 * Assumes image has already been transitioned to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
 * 
 * @param ctx Context.
 * @param src_buffer Source buffer.
 * @param dst_image Destination image.
 */
void lv_copy_buffer_to_image(
    lvContext *ctx,
    lvBuffer *src_buffer,
    lvImage *dst_image
);

void lv_copy_buffer_to_buffer(
    lvContext *ctx,
    lvBuffer *src_buffer,
    lvBuffer *dst_buffer,
    VkDeviceSize size
);


#endif // LAVA_VK_HELPERS_H