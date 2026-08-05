#ifndef LAVA_VK_GRAPHICS_PIPELINE_H
#define LAVA_VK_GRAPHICS_PIPELINE_H

#include "lava/internal.h"
#include "lava/vk/context.h"
#include "lava/containers/array.h"
#include "lava/containers/refarray.h"
#include "lava/vk/resource.h"
#include "lava/world/scene.h"


#define LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH 32

typedef struct {
    char name[LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH];
    lvResourceType type;
    lvResourceFreq freq;
    VkShaderStageFlagBits stages;
    uint32_t binding;
    size_t size;
    lvImage image;
} lvGraphicsPipelineResourceDefinition;

typedef struct {
    lvContext *ctx;
    lvScene *scene;

    size_t n_models;
    size_t n_materials;
    lvMaterial *materials;

    lvArray shader_modules;
    lvArray shader_stage_infos;

    size_t set0_accumulate;
    size_t set1_accumulate;
    size_t set2_accumulate;
    size_t set3_accumulate;

    lvArray resource_defs;
} lvGraphicsPipelineBuilder;

lvGraphicsPipelineBuilder lvGraphicsPipelineBuilder_new(
    lvContext *ctx,
    lvScene *scene
);

int lvGraphicsPipelineBuilder_load_shader(
    lvGraphicsPipelineBuilder *builder,
    const char *filepath,
    VkShaderStageFlagBits stage
);

/**
 * @brief Define a new resource to be created when the graphics pipeline is built.
 * 
 * Buffer resources are immediately mapped. They are unmapped upon destroying the pipeline.
 * 
 * @param builder Pointer to lvGraphicsPipelineBuilder.
 * @param name Name of the resource.
 * @param type Type of the resource.
 * @param freq Update frequencey of the resource.
 * @param stages Shader stages this resource is accessed from.
 * @param size Size of the resource (only for uniforms, leave 0 for samplers).
 * @return `0` if successful.
 */
int lvGraphicsPipelineBuilder_define_resource(
    lvGraphicsPipelineBuilder *builder,
    char name[LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH],
    lvResourceType type,
    lvResourceFreq freq,
    VkShaderStageFlagBits stages,
    size_t size
);


typedef struct {
    char name[LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH];
    size_t idx[2];
    lvBuffer buffer;
    void *mapped;
    size_t size;
} lvGraphicsPipelineUniformSlot;

typedef struct {
    char name[LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH];
    size_t idx[2];
    lvImage image;
} lvGraphicsPipelineSamplerSlot;

typedef struct {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorPool desc_pool;

    VkDescriptorSetLayout global_lyt;
    VkDescriptorSetLayout material_lyt;
    VkDescriptorSetLayout object_lyt;
    VkDescriptorSetLayout static_lyt;

    VkDescriptorSet *global_sets;
    VkDescriptorSet *material_sets;
    VkDescriptorSet *object_sets;
    VkDescriptorSet *static_sets;

    // TODO: Needs a name -> slot hashmap, but this is OK for now...
    lvRefArray uniform_slots;
    lvRefArray sampler_slots;
} lvGraphicsPipeline;

int lvGraphicsPipelineBuilder_build(
    lvGraphicsPipelineBuilder *builder,
    lvGraphicsPipeline *pipeline
);

int lvGraphicsPipeline_init(
    lvGraphicsPipeline *pipeline,
    lvContext *ctx,
    size_t swapchain_idx,
    const char *vertex_shader_filepath,
    const char *fragment_shader_filepath,
    lvRefArray *meshes,
    lvArray *uniforms,
    lvArray *descriptor_bindings
);

void lvGraphicsPipeline_free(lvGraphicsPipeline *pipeline, lvContext *ctx);

int lvGraphicsPipeline_set_uniform(
    lvGraphicsPipeline *pipeline,
    const char *name,
    void *data,
    size_t idx[2]
);

int lvGraphicsPipeline_bind_sampler(
    lvGraphicsPipeline *pipeline,
    VkCommandBuffer cmd_buf,
    const char *name
);


#endif // LAVA_VK_GRAPHICS_PIPELINE_H