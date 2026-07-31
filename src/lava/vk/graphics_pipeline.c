#include "lava/vk/graphics_pipeline.h"
#include "lava/core/io.h"
#include "lava/core/log.h"
#include "lava/containers/array.h"
#include "lava/vk/buffer.h"
#include "lava/vk/swapchain.h"



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


int lvGraphicsPipeline_init(
    lvGraphicsPipeline *pipeline,
    lvContext *ctx,
    size_t swapchain_idx,
    const char *vertex_shader_filepath,
    const char *fragment_shader_filepath,
    lvRefArray *buffers,
    lvArray *uniforms,
    lvArray *descriptor_bindings
) {
    // SHADERS

    VkShaderModule vert_module = create_shader_module(ctx, vertex_shader_filepath);
    VkShaderModule frag_module = create_shader_module(ctx, fragment_shader_filepath);


    // VERTEX BINDING

    // VkVertexInputBindingDescription vertex_binding_desc ={
    //     .binding = 0,
    //     .stride = sizeof(Vertex),
    //     .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    // };

    // VkVertexInputAttributeDescription vertex_attr_descs[2];
    // vertex_attr_descs[0] = (VkVertexInputAttributeDescription){
    //     .binding = 0,
    //     .location = 0,
    //     .format = VK_FORMAT_R32G32_SFLOAT,
    //     .offset = offsetof(Vertex, position)
    // };
    // vertex_attr_descs[1] = (VkVertexInputAttributeDescription){
    //     .binding = 0,
    //     .location = 1,
    //     .format = VK_FORMAT_R32G32B32A32_SFLOAT,
    //     .offset = offsetof(Vertex, color)
    // };

    lvArray bindings = lvArray_new(sizeof(VkVertexInputBindingDescription));
    lvArray attrs = lvArray_new(sizeof(VkVertexInputAttributeDescription));

    for (size_t i = 0; i < buffers->size; i++) {
        lvBuffer *buffer = buffers->data[i];

        lvArray_add(&bindings, &buffer->_binding_desc);
        lvArray_add(&attrs, &buffer->_attr_desc);
    }

    printf(
        "Graphics pipeline:\n"
        "- Bindings: %zu\n"
        "- Attrs:    %zu\n"
        "\n",
        bindings.size,
        attrs.size
    );


    // FIXED PIPELINE

    VkPipelineShaderStageCreateInfo vert_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vert_module,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo frag_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = frag_module,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo stage_infos[2] = {vert_stage_info, frag_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = NULL,
        .vertexBindingDescriptionCount = bindings.size,
        .pVertexBindingDescriptions = (VkVertexInputBindingDescription *)bindings.data,
        .vertexAttributeDescriptionCount = attrs.size,
        .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription *)attrs.data,
    };

    VkPipelineInputAssemblyStateCreateInfo input_ass_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .primitiveRestartEnable = VK_FALSE,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

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


    // DESCRIPTORS & LAYOUTS

    const uint32_t frame_lag = 2;

    VkDescriptorPoolSize desc_pool_size = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = frame_lag
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = descriptor_bindings->size,
        .pBindings = (VkDescriptorSetLayoutBinding *)descriptor_bindings->data
    };

    if (vkCreateDescriptorSetLayout(ctx->device, &descriptor_set_lyt_info, NULL, &pipeline->set_lyt) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout.\n");
        return 1;
    }

    VkDescriptorPoolCreateInfo desc_pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .poolSizeCount = 1,
        .pPoolSizes = &desc_pool_size,
        .maxSets = frame_lag
    };

    if (vkCreateDescriptorPool(ctx->device, &desc_pool_info, NULL, &pipeline->desc_pool) != VK_SUCCESS) {
        printf("Failed to create descriptor pool.\n");
        return 1;
    }

    lvArray desc_layouts = lvArray_new(sizeof(VkDescriptorSetLayout));
    for (size_t i = 0; i < frame_lag; i++) {
        lvArray_add(&desc_layouts, &pipeline->set_lyt);
    }

    VkDescriptorSetAllocateInfo desc_set_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = pipeline->desc_pool,
        .descriptorSetCount = frame_lag,
        .pSetLayouts = (VkDescriptorSetLayout *)desc_layouts.data
    };

    pipeline->desc_sets = lvArray_new(sizeof(VkDescriptorSet));
    pipeline->desc_sets.size = frame_lag;
    lvArray_resize(&pipeline->desc_sets);

    if (vkAllocateDescriptorSets(ctx->device, &desc_set_alloc_info, (VkDescriptorSet *)pipeline->desc_sets.data) != VK_SUCCESS) {
        printf("Failed to allocate descriptior set.\n");
        return 1;
    }

    for (size_t i = 0; i < frame_lag; i++) {
        VkDescriptorBufferInfo desc_buffer_info = {
            .buffer = LV_ARRAY_PTR_AT(uniforms, i, lvBuffer)->_buffer,
            .offset = 0,
            .range = VK_WHOLE_SIZE
        };

        VkWriteDescriptorSet desc_write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = LV_ARRAY_AT(&pipeline->desc_sets, i, VkDescriptorSet),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &desc_buffer_info,
            .pImageInfo = NULL,
            .pTexelBufferView = NULL
        };

        vkUpdateDescriptorSets(ctx->device, 1, &desc_write, 0, NULL);
    }

    VkPipelineLayoutCreateInfo pipeline_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &pipeline->set_lyt,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    if (vkCreatePipelineLayout(ctx->device, &pipeline_lyt_info, NULL, &pipeline->layout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout.");
        return 1;
    }

    
    // GRAPHICS PIPELINE

    VkPipelineRenderingCreateInfo pipeline_rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchain->format.format
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_rendering_info,
        .flags = 0,

        // Shader stages
        .stageCount = 2,
        .pStages = stage_infos,

        // Fixed pipeline
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_ass_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterizer_info,
        .pMultisampleState = &multisampling_info,
        .pDepthStencilState = NULL,
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

    vkDestroyShaderModule(ctx->device, frag_module, NULL);
    vkDestroyShaderModule(ctx->device, vert_module, NULL);

    lvArray_free(&bindings);
    lvArray_free(&attrs);

    return 0;
}

void lvGraphicsPipeline_free(lvGraphicsPipeline *pipeline, lvContext *ctx) {
    if (!pipeline) return;

    lvArray_free(&pipeline->desc_sets);
    vkDestroyDescriptorPool(ctx->device, pipeline->desc_pool, NULL);
    vkDestroyDescriptorSetLayout(ctx->device, pipeline->set_lyt, NULL);
    vkDestroyPipelineLayout(ctx->device, pipeline->layout, NULL);
    vkDestroyPipeline(ctx->device, pipeline->pipeline, NULL);
}