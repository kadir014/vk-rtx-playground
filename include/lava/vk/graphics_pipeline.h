#ifndef LAVA_VK_GRAPHICS_PIPELINE_H
#define LAVA_VK_GRAPHICS_PIPELINE_H

#include "lava/internal.h"
#include "lava/vk/context.h"
#include "lava/containers/array.h"
#include "lava/containers/refarray.h"


typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorSetLayout set_lyt;
    VkDescriptorPool desc_pool;
    lvArray desc_sets;
} lvGraphicsPipeline;

int lvGraphicsPipeline_init(
    lvGraphicsPipeline *pipeline,
    lvContext *ctx,
    size_t swapchain_idx,
    const char *vertex_shader_filepath,
    const char *fragment_shader_filepath,
    lvRefArray *buffers,
    lvArray *uniforms,
    lvArray *descriptor_bindings,
    VkImageView texture_view,
    VkSampler texture_sampler
);

void lvGraphicsPipeline_free(lvGraphicsPipeline *pipeline, lvContext *ctx);


#endif // LAVA_VK_GRAPHICS_PIPELINE_H