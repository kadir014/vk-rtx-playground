#include "lava/vk/graphics_pipeline.h"
#include "lava/core/io.h"
#include "lava/core/log.h"
#include "lava/containers/array.h"
#include "lava/vk/buffer.h"
#include "lava/vk/image.h"
#include "lava/vk/swapchain.h"

#include "lava/world/mesh.h"
#include "lava/world/model.h"


static VkShaderModule create_shader_module(lvContext *ctx, const char *filepath) {
    lvFileContent shader_source = lv_read_file_raw(filepath);
    if (!shader_source.data) {
        // TODO: fatal değil düzgün error handling internal fonksyino
        lv_fatal("Failed to read shader file: %s", filepath);
    }

    // TODO codeSize zero-terminated length mi istiyor (length+1) yoksa normal length mi?
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = shader_source.length,
        .pCode = (uint32_t *)shader_source.data
    };

    VkShaderModule shader_module;
    if (vkCreateShaderModule(ctx->device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        lv_fatal("Failed to create shader module.");
    }

    LV_FREE(shader_source.data);

    return shader_module;
}


lvGraphicsPipelineBuilder lvGraphicsPipelineBuilder_new(
    lvContext *ctx,
    lvScene *scene
) {
    lvGraphicsPipelineBuilder builder = {0};
    builder.ctx = ctx;
    builder.scene = scene;

    builder.n_models = scene->models.size;
    builder.n_materials = scene->materials.size;
    builder.materials = scene->materials.data;

    builder.shader_modules = lvArray_new(sizeof(VkShaderModule));
    builder.shader_stage_infos = lvArray_new(sizeof(VkPipelineShaderStageCreateInfo));

    builder.resource_defs = lvArray_new(sizeof(lvGraphicsPipelineResourceDefinition));

    return builder;
}

int lvGraphicsPipelineBuilder_load_shader(
    lvGraphicsPipelineBuilder *builder,
    const char *filepath,
    VkShaderStageFlagBits stage
) {
    VkShaderModule module = create_shader_module(builder->ctx, filepath);

    VkPipelineShaderStageCreateInfo stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = stage,
        .module = module,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    if (lvArray_add(&builder->shader_modules, &module) != 0) return 1;
    if (lvArray_add(&builder->shader_stage_infos, &stage_info) != 0) return 1;

    return 0;
}

int lvGraphicsPipelineBuilder_define_resource(
    lvGraphicsPipelineBuilder *builder,
    char name[LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH],
    lvResourceType type,
    lvResourceFreq freq,
    VkShaderStageFlagBits stages,
    size_t size
) {
    size_t binding = 0;
    if (freq == LV_RESOURCE_FREQ_GLOBAL) {
        binding = builder->set0_accumulate++;
    }
    else if (freq == LV_RESOURCE_FREQ_MATERIAL) {
        binding = builder->set1_accumulate++;
    }
    else if (freq == LV_RESOURCE_FREQ_OBJECT) {
        binding = builder->set2_accumulate++;
    }
    else if (freq == LV_RESOURCE_FREQ_STATIC) {
        binding = builder->set3_accumulate++;
    }

    lvGraphicsPipelineResourceDefinition resource_def = {
        .name = 0,
        .type = type,
        .freq = freq,
        .stages = stages,
        .binding = binding,
        .size = size
    };
    memcpy(resource_def.name, name, sizeof(char) * LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH);

    if (lvArray_add(&builder->resource_defs, &resource_def) != 0) return 1;

    return 0;
}

int lvGraphicsPipelineBuilder_build(
    lvGraphicsPipelineBuilder *builder,
    lvGraphicsPipeline *pipeline
) {
    lvContext *ctx = builder->ctx;
    uint32_t frame_lag = ctx->frame_lag;
    uint32_t n_models = builder->n_models;
    uint32_t n_materials = builder->n_materials;

    // BUILD DESCRIPTOR LAYOUTS FROM RESOURCE DEFINITIONS

    lvArray global_set_bindings = lvArray_new(sizeof(VkDescriptorSetLayoutBinding));
    lvArray material_set_bindings = lvArray_new(sizeof(VkDescriptorSetLayoutBinding));
    lvArray object_set_bindings = lvArray_new(sizeof(VkDescriptorSetLayoutBinding));
    lvArray static_set_bindings = lvArray_new(sizeof(VkDescriptorSetLayoutBinding));

    size_t global_copies   = builder->set0_accumulate > 0 ? frame_lag : 0;
    size_t material_copies = builder->set1_accumulate > 0 ? 1 * n_materials : 0;
    size_t object_copies   = builder->set2_accumulate > 0 ? frame_lag * n_models : 0;
    size_t static_copies   = builder->set3_accumulate > 0 ? 1 : 0;

    printf(
        "Beginning descriptor layout builds:\n"
        "- Global copies:   %zu\n"
        "- Material copies: %zu\n"
        "- Object copies:   %zu\n"
        "- Static copies:   %zu\n"
        "\n",
        global_copies,
        material_copies,
        object_copies,
        static_copies
    );

    lvArray desc_pool_sizes = lvArray_new(sizeof(VkDescriptorPoolSize));
    size_t total_copies = 0;

    for (size_t i = 0; i < builder->resource_defs.size; i++) {
        lvGraphicsPipelineResourceDefinition resource_def = LV_ARRAY_AT(
            &builder->resource_defs,
            i,
            lvGraphicsPipelineResourceDefinition
        );

        VkDescriptorType desc_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

        if (resource_def.type == LV_RESOURCE_TYPE_UNIFORM) {
            desc_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
        else if (resource_def.type == LV_RESOURCE_TYPE_SAMPLER) {
            desc_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        }

        VkDescriptorSetLayoutBinding desc_binding = {
            .binding = resource_def.binding,
            .descriptorType = desc_type,
            .descriptorCount = 1,
            .stageFlags = resource_def.stages,
            .pImmutableSamplers = NULL
        };

        size_t copies = 0;
        if (resource_def.freq == LV_RESOURCE_FREQ_GLOBAL) {
            lvArray_add(&global_set_bindings, &desc_binding);
            copies = global_copies;
        }
        else if (resource_def.freq == LV_RESOURCE_FREQ_MATERIAL) {
            lvArray_add(&material_set_bindings, &desc_binding);
            copies = material_copies;
        }
        else if (resource_def.freq == LV_RESOURCE_FREQ_OBJECT) {
            lvArray_add(&object_set_bindings, &desc_binding);
            copies = object_copies;
        }
        else if (resource_def.freq == LV_RESOURCE_FREQ_STATIC) {
            lvArray_add(&static_set_bindings, &desc_binding);
            copies = static_copies;
        }
        else {
            lv_fatal("Unknown resource frequency!");
        }

        // Allocating with 0 pool size is not allowed unless requested explicitly with extensions, but I'm not handling that shit
        if (copies == 0) {
            continue;
        }

        VkDescriptorPoolSize desc_pool_size = {
            .type = desc_type,
            .descriptorCount = copies
        };

        total_copies += copies;
        lvArray_add(&desc_pool_sizes, &desc_pool_size);
    }

    VkDescriptorPoolCreateInfo desc_pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .poolSizeCount = desc_pool_sizes.size,
        .pPoolSizes = (VkDescriptorPoolSize *)desc_pool_sizes.data,
        .maxSets = total_copies
    };

    if (vkCreateDescriptorPool(ctx->device, &desc_pool_info, NULL, &pipeline->desc_pool) != VK_SUCCESS) {
        printf("Failed to create descriptor pool.\n");
        return 1;
    }

    VkDescriptorSetLayoutCreateInfo global_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = builder->set0_accumulate,
        .pBindings = (VkDescriptorSetLayoutBinding *)global_set_bindings.data
    };

    VkDescriptorSetLayoutCreateInfo material_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = builder->set1_accumulate,
        .pBindings = (VkDescriptorSetLayoutBinding *)material_set_bindings.data
    };

    VkDescriptorSetLayoutCreateInfo object_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = builder->set2_accumulate,
        .pBindings = (VkDescriptorSetLayoutBinding *)object_set_bindings.data
    };

    VkDescriptorSetLayoutCreateInfo static_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = builder->set3_accumulate,
        .pBindings = (VkDescriptorSetLayoutBinding *)static_set_bindings.data
    };

    if (vkCreateDescriptorSetLayout(ctx->device, &global_lyt_info, NULL, &pipeline->global_lyt) != VK_SUCCESS) {
        printf("Failed to create GLOBAL descriptor set layout.\n");
        return 1;
    }

    if (vkCreateDescriptorSetLayout(ctx->device, &material_lyt_info, NULL, &pipeline->material_lyt) != VK_SUCCESS) {
        printf("Failed to create MATERIAL descriptor set layout.\n");
        return 1;
    }

    if (vkCreateDescriptorSetLayout(ctx->device, &object_lyt_info, NULL, &pipeline->object_lyt) != VK_SUCCESS) {
        printf("Failed to create OBJECT descriptor set layout.\n");
        return 1;
    }

    if (vkCreateDescriptorSetLayout(ctx->device, &static_lyt_info, NULL, &pipeline->static_lyt) != VK_SUCCESS) {
        printf("Failed to create STATIC descriptor set layout.\n");
        return 1;
    }

    printf(
        "Resource descriptors:\n"
        "- Global resources:        %zu\n"
        "- Material resources:      %zu\n"
        "- Object resources:        %zu\n"
        "- Static resources:        %zu\n"
        "- Total copies (max sets): %zu\n"
        "\n",
        builder->set0_accumulate * global_copies,
        builder->set1_accumulate * material_copies,
        builder->set2_accumulate * object_copies,
        builder->set3_accumulate * static_copies,
        total_copies
    );


    // BUILD DESCRIPTOR SETS

    VkDescriptorSetLayout *global_layouts = LV_MALLOC(sizeof(VkDescriptorSetLayout) * global_copies);
    for (uint32_t i = 0; i < global_copies; i++) {
        global_layouts[i] = pipeline->global_lyt;
    }

    VkDescriptorSetLayout *material_layouts = LV_MALLOC(sizeof(VkDescriptorSetLayout) * material_copies);
    for (uint32_t i = 0; i < material_copies; i++) {
        material_layouts[i] = pipeline->material_lyt;
    }

    VkDescriptorSetLayout *object_layouts = LV_MALLOC(sizeof(VkDescriptorSetLayout) * object_copies);
    for (uint32_t i = 0; i < object_copies; i++) {
        object_layouts[i] = pipeline->object_lyt;
    }

    VkDescriptorSetLayout *static_layouts = LV_MALLOC(sizeof(VkDescriptorSetLayout) * static_copies);
    for (uint32_t i = 0; i < static_copies; i++) {
        static_layouts[i] = pipeline->static_lyt;
    }

    VkDescriptorSetAllocateInfo global_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = pipeline->desc_pool,
        .descriptorSetCount = global_copies,
        .pSetLayouts = global_layouts
    };

    VkDescriptorSetAllocateInfo material_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = pipeline->desc_pool,
        .descriptorSetCount = material_copies,
        .pSetLayouts = material_layouts
    };

    VkDescriptorSetAllocateInfo object_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = pipeline->desc_pool,
        .descriptorSetCount = object_copies,
        .pSetLayouts = object_layouts
    };

    VkDescriptorSetAllocateInfo static_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = pipeline->desc_pool,
        .descriptorSetCount = static_copies,
        .pSetLayouts = static_layouts
    };

    pipeline->global_sets = LV_MALLOC(sizeof(VkDescriptorSet) * global_copies);
    pipeline->material_sets = LV_MALLOC(sizeof(VkDescriptorSet) * material_copies);
    pipeline->object_sets = LV_MALLOC(sizeof(VkDescriptorSet) * object_copies);
    pipeline->static_sets = LV_MALLOC(sizeof(VkDescriptorSet) * static_copies);

    if (global_set_alloc_info.descriptorSetCount > 0) {
        if (vkAllocateDescriptorSets(ctx->device, &global_set_alloc_info, pipeline->global_sets) != VK_SUCCESS) {
            printf("Failed to allocate descriptior set for GLOBAL.\n");
            return 1;
        }
    }

    if (material_set_alloc_info.descriptorSetCount > 0) {
        if (vkAllocateDescriptorSets(ctx->device, &material_set_alloc_info, pipeline->material_sets) != VK_SUCCESS) {
            printf("Failed to allocate descriptior set for MATERIAL.\n");
            return 1;
        }
    }

    if (object_set_alloc_info.descriptorSetCount > 0) {
        VkResult a = vkAllocateDescriptorSets(ctx->device, &object_set_alloc_info, pipeline->object_sets);
        if (a != VK_SUCCESS) {
            printf("Failed to allocate descriptior set for OBJECT: %d\n", a);
            return 1;
        }
    }

    if (static_set_alloc_info.descriptorSetCount > 0) {
        if (vkAllocateDescriptorSets(ctx->device, &static_set_alloc_info, pipeline->static_sets) != VK_SUCCESS) {
            printf("Failed to allocate descriptior set for STATIC.\n");
            return 1;
        }
    }

    LV_FREE(global_layouts);
    LV_FREE(material_layouts);
    LV_FREE(object_layouts);
    LV_FREE(static_layouts);


    // ALLOCATE ACTUAL RESOURCES

    // TODO: There might be a more efficient solution than looping over resource
    // definitions again, but I'm happy with this for now
    
    pipeline->uniform_slots = lvRefArray_new();
    pipeline->sampler_slots = lvRefArray_new();

    for (size_t i = 0; i < builder->resource_defs.size; i++) {
        lvGraphicsPipelineResourceDefinition resource_def = LV_ARRAY_AT(
            &builder->resource_defs,
            i,
            lvGraphicsPipelineResourceDefinition
        );

        size_t copies = 0;
        if (resource_def.freq == LV_RESOURCE_FREQ_GLOBAL) {
            copies = global_copies;
        }
        else if (resource_def.freq == LV_RESOURCE_FREQ_MATERIAL) {
            copies = material_copies;
        }
        else if (resource_def.freq == LV_RESOURCE_FREQ_OBJECT) {
            copies = object_copies;
        }
        else if (resource_def.freq == LV_RESOURCE_FREQ_STATIC) {
            copies = static_copies;
        }

        // Allocating with 0 pool size is not allowed unless requested explicitly with extensions, but I'm not handling that shit
        if (copies == 0) {
            continue;
        }

        // Allocate buffer resources

        if (resource_def.type == LV_RESOURCE_TYPE_UNIFORM) {
            size_t w = 0;
            size_t h = 0;
            VkDescriptorSet *uniform_sets = NULL;

            switch (resource_def.freq) {
                case LV_RESOURCE_FREQ_GLOBAL:
                    uniform_sets = pipeline->global_sets;
                    w = frame_lag;
                    h = frame_lag;
                    break;
                case LV_RESOURCE_FREQ_MATERIAL:
                    uniform_sets = pipeline->material_sets;
                    w = n_materials;
                    h = 1;
                    break;
                case LV_RESOURCE_FREQ_OBJECT:
                    uniform_sets = pipeline->object_sets;
                    w = n_models;
                    h = frame_lag;
                    break;
                case LV_RESOURCE_FREQ_STATIC:
                    uniform_sets = pipeline->static_sets;
                    w = 1;
                    h = 1;
                    break;
            }

            for (size_t y = 0; y < h; y++) {
                for (size_t x = 0; x < w; x++) {

                    lvGraphicsPipelineUniformSlot *uniform_slot = LV_MALLOC(sizeof(lvGraphicsPipelineUniformSlot));
                    if (!uniform_slot) {
                        printf("Failed to allocate for uniform slot.\n");
                        return 1;
                    }

                    size_t idx2d[2] = {y, x};
                    memcpy(uniform_slot->idx, idx2d, sizeof(idx2d) * 2);

                    memcpy(uniform_slot->name, resource_def.name, sizeof(char) * LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH);

                    uniform_slot->size = resource_def.size;
                    uniform_slot->buffer = (lvBuffer){0};
                    uniform_slot->mapped = NULL;

                    VkBufferCreateInfo uniform_buffer_info = {
                        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                        .pNext = NULL,
                        .flags = 0,
                        .size = resource_def.size,
                        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
                    };

                    // TODO: Uniform buffers can be read, written and accessed in random order. But maybe choose sequential write for speed?
                    VmaAllocationCreateInfo uniform_buffer_alloc_info = {
                        .usage = VMA_MEMORY_USAGE_AUTO,
                        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                    };

                    if (
                        vmaCreateBuffer(
                            ctx->allocator,
                            &uniform_buffer_info,
                            &uniform_buffer_alloc_info,
                            &uniform_slot->buffer._buffer,
                            &uniform_slot->buffer._allocation,
                            NULL
                        ) != VK_SUCCESS
                    ) {
                        printf("Failed to create uniform buffer.\n");
                        return 1;
                    }

                    if (
                        vmaMapMemory(
                            ctx->allocator,
                            uniform_slot->buffer._allocation,
                            &uniform_slot->mapped
                        ) != VK_SUCCESS
                    ) {
                        printf("Failed to map uniform buffer.\n");
                        return 1;
                    }

                    lvRefArray_add(&pipeline->uniform_slots, uniform_slot);

                    size_t set_idx = y * w + x;

                    VkDescriptorBufferInfo desc_buffer_info = {
                        .buffer = uniform_slot->buffer._buffer,
                        .offset = 0,
                        .range = VK_WHOLE_SIZE
                    };

                    VkWriteDescriptorSet desc_write = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = NULL,
                        .dstSet = uniform_sets[set_idx],
                        .dstBinding = resource_def.binding,
                        .dstArrayElement = 0,
                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        .descriptorCount = 1,
                        .pBufferInfo = &desc_buffer_info,
                        .pImageInfo = NULL,
                        .pTexelBufferView = NULL
                    };

                    vkUpdateDescriptorSets(ctx->device, 1, &desc_write, 0, NULL);
                }
            }
        }
        else if (resource_def.type == LV_RESOURCE_TYPE_SAMPLER) {
            size_t w = 0;
            size_t h = 0;
            VkDescriptorSet *sampler_sets = NULL;

            switch (resource_def.freq) {
                case LV_RESOURCE_FREQ_GLOBAL:
                    sampler_sets = pipeline->global_sets;
                    w = frame_lag;
                    h = frame_lag;
                    break;
                case LV_RESOURCE_FREQ_MATERIAL:
                    sampler_sets = pipeline->material_sets;
                    w = n_materials;
                    h = 1;
                    break;
                case LV_RESOURCE_FREQ_OBJECT:
                    sampler_sets = pipeline->object_sets;
                    w = n_models;
                    h = frame_lag;
                    break;
                case LV_RESOURCE_FREQ_STATIC:
                    sampler_sets = pipeline->static_sets;
                    w = 1;
                    h = 1;
                    break;
            }

            for (size_t y = 0; y < h; y++) {
                for (size_t x = 0; x < w; x++) {

                    lvGraphicsPipelineSamplerSlot *sampler_slot = LV_MALLOC(sizeof(lvGraphicsPipelineSamplerSlot));
                    if (!sampler_slot) {
                        printf("Failed to allocate for sampler slot.\n");
                        return 1;
                    }

                    size_t idx2d[2] = {y, x};
                    memcpy(sampler_slot->idx, idx2d, sizeof(idx2d) * 2);

                    char *material_name_at_x = builder->materials[x].name;
                    memcpy(sampler_slot->name, material_name_at_x, sizeof(char) * LV_GRAPHICS_PIPELINE_RESOURCE_NAME_LENGTH);

                    sampler_slot->image = builder->materials[x].image;

                    VkDescriptorImageInfo desc_image_info = {
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        .imageView = sampler_slot->image.view,
                        .sampler = sampler_slot->image.sampler,
                    };

                    lvRefArray_add(&pipeline->sampler_slots, sampler_slot);

                    size_t set_idx = y * w + x;

                    VkWriteDescriptorSet desc_write_img = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .pNext = NULL,
                        .dstSet = sampler_sets[set_idx],
                        .dstBinding = resource_def.binding,
                        .dstArrayElement = 0,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 1,
                        .pBufferInfo = NULL,
                        .pImageInfo = &desc_image_info,
                        .pTexelBufferView = NULL
                    };

                    vkUpdateDescriptorSets(ctx->device, 1, &desc_write_img, 0, NULL);
                }
            }
        }
    }


    // COLLECT VERTEX BUFFER BINDINGS AND ATTRIBUTES

    lvArray vertex_bindings = lvArray_new(sizeof(VkVertexInputBindingDescription));
    lvArray vertex_attrs = lvArray_new(sizeof(VkVertexInputAttributeDescription));

    /*
        Loop over one mesh because we only need the binding state of one model,
        since every model share the same binding and attribute layouts..

        TODO: I need a more elegant solution for this.
    */
    for (size_t i = 0; i < 1; i++) {
        lvModel *model = LV_ARRAY_PTR_AT(&builder->scene->models, i, lvModel);
        // TODO: Loop over all meshes of one model
        lvMesh *mesh = model->meshes.data[0];

        lvArray_add(&vertex_bindings, &mesh->vertices._binding_desc);
        lvArray_add(&vertex_attrs, &mesh->vertices._attr_desc);
        lvArray_add(&vertex_bindings, &mesh->uvs._binding_desc);
        lvArray_add(&vertex_attrs, &mesh->uvs._attr_desc);
        lvArray_add(&vertex_bindings, &mesh->normals._binding_desc);
        lvArray_add(&vertex_attrs, &mesh->normals._attr_desc);
    }


    // FIXED PIPELINE

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .vertexBindingDescriptionCount = vertex_bindings.size,
        .pVertexBindingDescriptions = (VkVertexInputBindingDescription *)vertex_bindings.data,
        .vertexAttributeDescriptionCount = vertex_attrs.size,
        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription *)vertex_attrs.data,
    };

    VkPipelineInputAssemblyStateCreateInfo input_ass_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .primitiveRestartEnable = VK_FALSE,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // TODO: swapchain_idx parameter
    const size_t swapchain_idx = 0;
    lvSwapchain *swapchain = ctx->swapchains.data[swapchain_idx];

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)swapchain->extent.width,
        .height = (float)swapchain->extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .extent = swapchain->extent,
        .offset = (VkOffset2D){0, 0}
    };

    VkPipelineViewportStateCreateInfo viewport_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .viewportCount = 1,
        .scissorCount = 1,
        .pViewports = &viewport,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizer_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
        .stencilTestEnable = VK_FALSE,
        .front = {0},
        .back = {0},
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
    };

    VkPipelineColorBlendStateCreateInfo color_blending_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
    };
    color_blending_info.blendConstants[0] = 0.0f;
    color_blending_info.blendConstants[1] = 0.0f;
    color_blending_info.blendConstants[2] = 0.0f;
    color_blending_info.blendConstants[3] = 0.0f;

    
    VkDescriptorSetLayout desc_layouts[4] = {
        pipeline->global_lyt,
        pipeline->material_lyt,
        pipeline->object_lyt,
        pipeline->static_lyt
    };
    VkPipelineLayoutCreateInfo pipeline_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 4,
        .pSetLayouts = desc_layouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    if (vkCreatePipelineLayout(ctx->device, &pipeline_lyt_info, NULL, &pipeline->layout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout.");
        return 1;
    }


    // INIT GRAPHICS PIPELINE

    VkPipelineRenderingCreateInfo pipeline_rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchain->format.format,
        .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_info,
        .flags = 0,

        // Shader stages
        .stageCount = builder->shader_stage_infos.size,
        .pStages = (VkPipelineShaderStageCreateInfo *)builder->shader_stage_infos.data,

        // Fixed pipeline
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_ass_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterizer_info,
        .pMultisampleState = &multisampling_info,
        .pDepthStencilState = &depth_stencil_info,
        .pColorBlendState = &color_blending_info,
        .pDynamicState = NULL,

        // Layout
        .layout = pipeline->layout,

        // Renderpass
        .renderPass = NULL,
        .subpass = 0,

        // For graphics pipeline derivation
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline->pipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline.");
        return 1;
    }


    // CLEANUP

    for (size_t i = 0; i < builder->shader_modules.size; i++) {
        vkDestroyShaderModule(
            ctx->device,
            LV_ARRAY_AT(&builder->shader_modules, i, VkShaderModule),
            NULL
        );
    }

    lvArray_free(&vertex_attrs);
    lvArray_free(&vertex_bindings);
    lvArray_free(&desc_pool_sizes);
    lvArray_free(&global_set_bindings);
    lvArray_free(&material_set_bindings);
    lvArray_free(&object_set_bindings);
    lvArray_free(&static_set_bindings);
    lvArray_free(&builder->resource_defs);
    lvArray_free(&builder->shader_modules);
    lvArray_free(&builder->shader_stage_infos);

    return 0;
}

void lvGraphicsPipeline_free(lvGraphicsPipeline *pipeline, lvContext *ctx) {
    if (!pipeline) return;

    for (size_t i = 0; i < pipeline->uniform_slots.size; i++) {
        lvGraphicsPipelineUniformSlot *uniform_slot = pipeline->uniform_slots.data[i];
        vmaUnmapMemory(ctx->allocator, uniform_slot->buffer._allocation);
        vmaDestroyBuffer(ctx->allocator, uniform_slot->buffer._buffer, uniform_slot->buffer._allocation);
        LV_FREE(uniform_slot);
    }
    lvRefArray_free(&pipeline->uniform_slots);

    for (size_t i = 0; i < pipeline->sampler_slots.size; i++) {
        lvGraphicsPipelineUniformSlot *sampler_slot = pipeline->sampler_slots.data[i];
        LV_FREE(sampler_slot);
    }
    lvRefArray_free(&pipeline->sampler_slots);

    LV_FREE(pipeline->global_sets);
    LV_FREE(pipeline->material_sets);
    LV_FREE(pipeline->object_sets);
    LV_FREE(pipeline->static_sets);

    vkDestroyDescriptorPool(ctx->device, pipeline->desc_pool, NULL);
    vkDestroyDescriptorSetLayout(ctx->device, pipeline->global_lyt, NULL);
    vkDestroyDescriptorSetLayout(ctx->device, pipeline->material_lyt, NULL);
    vkDestroyDescriptorSetLayout(ctx->device, pipeline->object_lyt, NULL);
    vkDestroyDescriptorSetLayout(ctx->device, pipeline->static_lyt, NULL);
    vkDestroyPipelineLayout(ctx->device, pipeline->layout, NULL);
    vkDestroyPipeline(ctx->device, pipeline->pipeline, NULL);
}

int lvGraphicsPipeline_set_uniform(
    lvGraphicsPipeline *pipeline,
    const char *name,
    void *data,
    size_t idx[2]
) {
    if (!pipeline || !name || !data) {
        return 1;
    }

    lvGraphicsPipelineUniformSlot *found_slot = NULL;
    for (size_t i = 0; i < pipeline->uniform_slots.size; i++) {
        lvGraphicsPipelineUniformSlot *slot = pipeline->uniform_slots.data[i];

        if (
            strcmp(slot->name, name) == 0 &&
            slot->idx[0] == idx[0] &&
            slot->idx[1] == idx[1]
        ) {
            found_slot = slot;
            break;
        }
    }

    if (!found_slot) {
        return 1;
    }

    size_t frame_idx = idx[0];
    size_t obj_idx = idx[1];

    memcpy(found_slot->mapped, data, found_slot->size);

    return 0;
}

int lvGraphicsPipeline_bind_sampler(
    lvGraphicsPipeline *pipeline,
    VkCommandBuffer cmd_buf,
    const char *name
) {
    lvGraphicsPipelineSamplerSlot *found_slot = NULL;
    for (size_t i = 0; i < pipeline->sampler_slots.size; i++) {
        lvGraphicsPipelineSamplerSlot *slot = pipeline->sampler_slots.data[i];

        if (
            strcmp(slot->name, name) == 0
            //slot->idx[0] == idx[0] &&
            //slot->idx[1] == idx[1]
        ) {
            found_slot = slot;
            break;
        }
    }

    if (!found_slot) {
        return 1;
    }

    size_t frame_idx = found_slot->idx[0];
    size_t mat_idx = found_slot->idx[1];
    // TODO:                     2 değil n_materials veya hangi FREQUENCY ISE
    size_t set_idx = frame_idx * 2 + mat_idx;

    vkCmdBindDescriptorSets(
        cmd_buf,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline->layout,
        1,              // firstSet TODO: GET FROM FREQUENCY
        1,              // set count
        &pipeline->material_sets[set_idx], // material_sets değil! hangi freq ise
        0,
        NULL
    );

    return 0;
}