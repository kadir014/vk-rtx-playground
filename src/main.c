#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "lava/lava.h"


#define INVALID_UINT32_IDX UINT32_MAX


/**
 * @brief Check that all requested validation layers are available.
 * 
 * @param requested_layers Array of requested validation layer names.
 * @param n_requested_layers Number of requested validation layer names.
 * @param available_layers Array of currently available validation layer names.
 * @param n_available_layers Number of currently available validation layer names.
 * @return `0` if all requested layers are available.
 *         `1` if at least one requested layer is missing.
 */
int check_layers(
    const char *const *requested_layers,
    uint32_t n_requested_layers,
    const VkLayerProperties *available_layers,
    uint32_t n_available_layers
) {
    for (uint32_t i = 0; i < n_requested_layers; i++) {
        bool layer_found = false;

        for (uint32_t j = 0; j < n_available_layers; j++) {
            if (strcmp(requested_layers[i], available_layers[j].layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return 1;
        }
    }

    return 0;
}


typedef struct {
    uint32_t graphics_idx;
    uint32_t present_idx;
} QueueFamilies;

typedef struct {
    VkSwapchainKHR swapchain;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode;
    VkExtent2D extent;
    lvArray images;
    lvArray views;
} Swapchain;

typedef struct {
    VkInstance inst;
    VkSurfaceKHR surface;

    VkPhysicalDevice phydevice;
    QueueFamilies families;

    VkDevice device;
    VkQueue graphics_q;
    VkQueue present_q;

    Swapchain swapchain;

    VkPipelineLayout pipeline_lyt;
    VkPipeline graphics_pipeline;
} Context;


/**
 * @brief Create a Vulkan instance.
 * 
 * @param ctx Pointer to Context
 * @param window Pointer to SDL_Window.
 * @return `0` if succesful.
 *         `1` if instance creation failed.
 *         `2` if requested validation layers are not available.
 *         `3` if surface creation failed.
 */
int create_instance(Context *ctx, SDL_Window *window) {
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "booty cheeks",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    uint32_t n_extensions = 0;
    const char * const *extension_names = NULL;
    SDL_Vulkan_GetInstanceExtensions(window, &n_extensions, NULL);
    extension_names = LV_MALLOC(sizeof(char *) * n_extensions);;
    SDL_Vulkan_GetInstanceExtensions(window, &n_extensions, (const char **)extension_names);

    printf("Found %u extensions:\n", n_extensions);
    for (uint32_t i = 0; i < n_extensions; i++) {
        printf("%u: %s\n", i, extension_names[i]);
    }
    printf("\n");

    const char * const requested_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    uint32_t n_requested_layers = sizeof(requested_layers) / sizeof(char *);

    uint32_t n_layers = 0;
    VkLayerProperties *layer_names = NULL;
    vkEnumerateInstanceLayerProperties(&n_layers, NULL);
    layer_names = LV_MALLOC(sizeof(VkLayerProperties) * n_layers);
    vkEnumerateInstanceLayerProperties(&n_layers, layer_names);

    printf("Found %u validation layers:\n", n_layers);
    for (uint32_t i = 0; i < n_layers; i++) {
        printf("%u: %s -- %s\n", i, layer_names[i].layerName, layer_names[i].description);
    }
    printf("\n");

    if (check_layers(requested_layers, n_requested_layers, layer_names, n_layers)) {
        // TODO: nv_set_error
        printf("Requested validation layers are not available on the system.");
        LV_FREE(extension_names);
        LV_FREE(layer_names);
        return 2;
    }
    else {
        printf("Applied %u validation layers:\n", n_requested_layers);
        for (uint32_t i = 0; i < n_requested_layers; i++) {
            printf("%u: %s\n", i, requested_layers[i]);
        }
        printf("\n");
    }

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = n_extensions,
        .ppEnabledExtensionNames = extension_names,
        .enabledLayerCount = n_requested_layers,
        .ppEnabledLayerNames = requested_layers,
        .flags = 0
    };

    int ret = 0;

    if (vkCreateInstance(&create_info, NULL, &ctx->inst) != VK_SUCCESS) {
        ret = 1;
    }

    if (!SDL_Vulkan_CreateSurface(window, ctx->inst, &ctx->surface)) {
        ret = 3;
    }

    LV_FREE(extension_names);
    LV_FREE(layer_names);

    return ret;
}

/**
 * @brief Fetch the physical device.
 * 
 * @param ctx Pointer to Context
 * @return `0` if succesful.
 *         `1` if no physical devices are found.
 */
int get_physical_device(Context *ctx) {
    uint32_t n_phydevices = 0;
    vkEnumeratePhysicalDevices(ctx->inst, &n_phydevices, NULL);

    if (n_phydevices == 0) {
        return 1;
    }

    VkPhysicalDevice *phydevices = LV_MALLOC(sizeof(VkPhysicalDevice) * n_phydevices);
    vkEnumeratePhysicalDevices(ctx->inst, &n_phydevices, phydevices);

    uint32_t discrete_idx = INVALID_UINT32_IDX;
    for (uint32_t i = 0; i < n_phydevices; i++) {
        VkPhysicalDeviceProperties phydevice_info;
        vkGetPhysicalDeviceProperties(phydevices[i], &phydevice_info);

        if (phydevice_info.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            discrete_idx = i;
            break;
        }
    }

    if (discrete_idx == INVALID_UINT32_IDX) {
        ctx->phydevice = phydevices[discrete_idx];
    }
    else {
        ctx->phydevice = phydevices[0];
    }

    LV_FREE(phydevices);

    VkPhysicalDeviceProperties phydevice_info;
    vkGetPhysicalDeviceProperties(ctx->phydevice, &phydevice_info);

    printf(
        "Chosen physical device properties:\n"
        "- Name: %s\n"
        "- Max 2D image dimension:        %u\n"
        "- Max framebuffer color samples: %u\n"
        "- Max framebuffer resolution:    %ux%u\n"
        "- Max vertex input attributes:   %u\n"
        "- Max memory allocations:        %u\n"
        "\n",
        phydevice_info.deviceName,
        phydevice_info.limits.maxImageDimension2D,
        phydevice_info.limits.framebufferColorSampleCounts,
        phydevice_info.limits.maxFramebufferWidth,
        phydevice_info.limits.maxFramebufferHeight,
        phydevice_info.limits.maxVertexInputAttributes,
        phydevice_info.limits.maxMemoryAllocationCount
    );

    return 0;
}


QueueFamilies find_queue_families(VkPhysicalDevice phydevice, VkSurfaceKHR surface) {
    QueueFamilies family_indices = {
        .graphics_idx = INVALID_UINT32_IDX,
        .present_idx = INVALID_UINT32_IDX,
    };

    uint32_t n_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phydevice, &n_families, NULL);

    VkQueueFamilyProperties *families = LV_MALLOC(sizeof(VkQueueFamilyProperties) * n_families);
    vkGetPhysicalDeviceQueueFamilyProperties(phydevice, &n_families, families);

    for (uint32_t i = 0; i < n_families; i++) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            family_indices.graphics_idx = i;
        }

        VkBool32 present_supported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(phydevice, i, surface, &present_supported);

        if (present_supported) {
            family_indices.present_idx = i;
        }
    }

    printf(
        "Queue families:\n"
        "- Graphics index: %u\n"
        "- Present index: %u\n"
        "\n",
        family_indices.graphics_idx,
        family_indices.present_idx
    );

    LV_FREE(families);
    return family_indices;
}


void validate_physical_device(
    VkPhysicalDevice phydevice,
    const char **requested_extensions,
    uint32_t n_requested_extensions
) {
    uint32_t n_extensions = 0;
    vkEnumerateDeviceExtensionProperties(phydevice, NULL, &n_extensions, NULL);

    // TODO: Better error & memory handling
    if (n_extensions == 0) {
        lv_fatal("No extension is supported for this physical device.");
        return;
    }

    VkExtensionProperties *available_extensions = LV_MALLOC(sizeof(VkExtensionProperties) * n_extensions);
    vkEnumerateDeviceExtensionProperties(phydevice, NULL, &n_extensions, available_extensions);

    VkPhysicalDeviceDynamicRenderingFeatures dr = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = NULL,
        .dynamicRendering = VK_FALSE
    };

    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &dr
    };
    vkGetPhysicalDeviceFeatures2(phydevice, &features);

    printf("GPU supports dynamic rendering? %u\n\n", dr.dynamicRendering);


    // printf("Found %u available extensions for the current physical device:\n", n_extensions);
    // for (uint32_t i = 0; i < n_extensions; i++) {
    //     printf("%u: %s\n", i, available_extensions[i].extensionName);
    // }
    // printf("\n");

    for (uint32_t i = 0; i < n_requested_extensions; i++) {
        bool found = false;

        for (uint32_t j = 0; j < n_extensions; j++) {
            if (strcmp(requested_extensions[i], available_extensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            lv_fatal("Requested extensions are not supported on this physical device.");
            return;
        }
    }
}


int create_logical_device(Context *ctx) {
    #define N_REQUESTED_DEVICE_EXTENSIONS 3
    const char *requested_device_extensions[N_REQUESTED_DEVICE_EXTENSIONS] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
    };

    validate_physical_device(ctx->phydevice, requested_device_extensions, N_REQUESTED_DEVICE_EXTENSIONS);
    
    QueueFamilies families = find_queue_families(ctx->phydevice, ctx->surface);
    ctx->families = families;

    if (
        families.graphics_idx == INVALID_UINT32_IDX &&
        families.present_idx == INVALID_UINT32_IDX
    ) {
        printf("Requested queues are not found in the physical device.");
        return 2;
    }

    #define N_UNIQUE_FAMILIES 2

    uint32_t unique_families[N_UNIQUE_FAMILIES] = {
        families.graphics_idx,
        families.present_idx
    };

    VkDeviceQueueCreateInfo queue_create_infos[N_UNIQUE_FAMILIES];
    for (uint32_t i = 0; i < N_UNIQUE_FAMILIES; i++) {
        float queue_priority = 1.0f;
        queue_create_infos[i] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = NULL,
            .queueFamilyIndex = unique_families[i],
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };
    }

    VkPhysicalDeviceFeatures device_features = {false};

    // ENABLED FEATURES:
    // Dynamic rendering
    // Synchronization2

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = NULL,
        .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceSynchronization2Features sync2_feat = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynamic_rendering,
        .synchronization2 = VK_TRUE
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &sync2_feat,
        .queueCreateInfoCount = N_UNIQUE_FAMILIES,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &device_features,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = N_REQUESTED_DEVICE_EXTENSIONS,
        .ppEnabledExtensionNames = requested_device_extensions
    };

    if (vkCreateDevice(ctx->phydevice, &device_create_info, NULL, &ctx->device) != VK_SUCCESS) {
        printf("Failed to create logical device.");
        return 1;
    }

    vkGetDeviceQueue(ctx->device, families.graphics_idx, 0, &ctx->graphics_q);
    vkGetDeviceQueue(ctx->device, families.present_idx, 0, &ctx->present_q);

    return 0;
}


typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    lvArray formats;
    lvArray present_modes;
} SwapChainSupport;

SwapChainSupport get_swap_chain_support(
    VkPhysicalDevice phydevice,
    VkSurfaceKHR surface
) {
    SwapChainSupport sc;
    sc.formats = lvArray_new(sizeof(VkSurfaceFormatKHR));
    sc.present_modes = lvArray_new(sizeof(VkPresentModeKHR));

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        phydevice, surface, &sc.capabilities
    );

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        phydevice, surface, (uint32_t *)&sc.formats.size, NULL
    );

    if (sc.formats.capacity == 0) {
        lv_fatal("0 formats?");
        return sc;
    }

    lvArray_resize(&sc.formats);

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        phydevice,
        surface,
        (uint32_t *)&sc.formats.size,
        (VkSurfaceFormatKHR *)sc.formats.data
    );

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        phydevice,
        surface,
        (uint32_t *)&sc.present_modes.size,
        NULL
    );

    if (sc.present_modes.capacity == 0) {
        lv_fatal("0 present modes?");
        return sc;
    }

    lvArray_resize(&sc.present_modes);

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        phydevice,
        surface,
        (uint32_t *)&sc.present_modes.size,
        (VkPresentModeKHR *)sc.present_modes.data
    );

    return sc;
}

int create_swapchain(Context *ctx) {
    SwapChainSupport sc = get_swap_chain_support(ctx->phydevice, ctx->surface);

    VkSurfaceFormatKHR best_format = LV_ARRAY_AT(&sc.formats, 0, VkSurfaceFormatKHR);
    for (uint32_t i = 0; i < sc.formats.size; i++) {
        VkSurfaceFormatKHR format = LV_ARRAY_AT(&sc.formats, i, VkSurfaceFormatKHR);

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            best_format = format;
            break;
        }
    }

    // MAILBOX = triple buffering
    // FIFO = vsync
    VkPresentModeKHR best_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < sc.present_modes.size; i++) {
        VkPresentModeKHR present_mode = LV_ARRAY_AT(&sc.present_modes, i, VkPresentModeKHR);

        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            best_present_mode = present_mode;
            break;
        }
    }

    // TODO: Higher DPI stuff
    VkExtent2D best_extent = sc.capabilities.currentExtent;

    uint32_t n_swap_images = sc.capabilities.minImageCount + 1;
    if (sc.capabilities.maxImageCount > 0 && n_swap_images > sc.capabilities.maxImageCount) {
        n_swap_images = sc.capabilities.maxImageCount;
    }

    ctx->swapchain.extent = best_extent;
    ctx->swapchain.format = best_format;
    ctx->swapchain.present_mode = best_present_mode;

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .surface = ctx->surface,
        .minImageCount = n_swap_images,
        .imageFormat = best_format.format,
        .imageColorSpace = best_format.colorSpace,
        .imageExtent = best_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = sc.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = best_present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = NULL
    };

    #define N_UNIQUE_FAMILIES 2

    uint32_t unique_families[N_UNIQUE_FAMILIES] = {
        ctx->families.graphics_idx,
        ctx->families.present_idx
    };

    if (ctx->families.graphics_idx != ctx->families.present_idx) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = N_UNIQUE_FAMILIES;
        swapchain_create_info.pQueueFamilyIndices = unique_families;
    }
    else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = NULL;
    }

    if (vkCreateSwapchainKHR(ctx->device, &swapchain_create_info, NULL, &ctx->swapchain.swapchain) != VK_SUCCESS) {
        printf("Failed to create swapchain.");
        return 1;
    }

    // IMAGES

    ctx->swapchain.images = lvArray_new(sizeof(VkImage));

    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain.swapchain, (uint32_t *)&ctx->swapchain.images.size, NULL);

    if (ctx->swapchain.images.size == 0) {
        printf("0 swap chain images?");
        return 2;
    }

    lvArray_resize(&ctx->swapchain.images);
    vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain.swapchain, (uint32_t *)&ctx->swapchain.images.size, (VkImage *)(ctx->swapchain.images.data));

    printf(
        "Initialized swapchain:\n"
        "- Minimum images: %u\n"
        "- Allocated images: %zu\n"
        "- Extent: %ux%u\n"
        "\n",
        n_swap_images,
        ctx->swapchain.images.size,
        best_extent.width, best_extent.height
    );

    // IMAGE VIEWS

    ctx->swapchain.views = lvArray_new(sizeof(VkImageView));

    for (uint32_t i = 0; i < ctx->swapchain.images.size; i++) {
        VkImageView view = VK_NULL_HANDLE;
        lvArray_add(&ctx->swapchain.views, (void *)&view);

        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = LV_ARRAY_AT(&ctx->swapchain.images, i, VkImage),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = best_format.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };

        if (vkCreateImageView(ctx->device, &create_info, NULL, LV_ARRAY_PTR_AT(&ctx->swapchain.views, i, VkImageView)) != VK_SUCCESS) {
            printf("Failed to create image view.");
            return 3;
        } else {
            printf("view creation successful\n");
        }
    }

    lvArray_free(&sc.formats);
    lvArray_free(&sc.present_modes);

    printf("imgs size: %zu\n", ctx->swapchain.images.size);

    return 0;
}


VkShaderModule create_shader_module(VkDevice device, const char *filepath) {
    lvFileContent shader_source = lv_read_file_raw(filepath);
    if (!shader_source.data) {
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
    if (vkCreateShaderModule(device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        lv_fatal("Failed to create shader module.");
    }

    LV_FREE(shader_source.data);

    return shader_module;
}


int create_graphics_pipeline(Context *ctx) {
    // SHADERS

    VkShaderModule vert_module = create_shader_module(ctx->device, "../shaders/first.vert.spv");
    VkShaderModule frag_module = create_shader_module(ctx->device, "../shaders/first.frag.spv");


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
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
    };

    VkPipelineInputAssemblyStateCreateInfo input_ass_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .primitiveRestartEnable = VK_FALSE,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)ctx->swapchain.extent.width,
        .height = (float)ctx->swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .extent = ctx->swapchain.extent,
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
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
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

    VkPipelineLayoutCreateInfo pipeline_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    if (vkCreatePipelineLayout(ctx->device, &pipeline_lyt_info, NULL, &ctx->pipeline_lyt) != VK_SUCCESS) {
        printf("Failed to create pipeline layout.");
        return 1;
    }

    
    // GRAPHICS PIPELINE

    VkPipelineRenderingCreateInfo pipeline_rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &ctx->swapchain.format.format
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
        .layout = ctx->pipeline_lyt,

        // Renderpass
        .renderPass = NULL,
        .subpass = 0,

        // For graphics pipeline derivation
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &ctx->graphics_pipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline.");
        return 1;
    }

    vkDestroyShaderModule(ctx->device, frag_module, NULL);
    vkDestroyShaderModule(ctx->device, vert_module, NULL);

    return 0;
}


void transition_image_layout(
    VkCommandBuffer cmd_buf,
    uint32_t image_idx,
    VkImage *swapchain_images,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkAccessFlags2 src_access_mask,
    VkAccessFlags2 dst_access_mask,
    VkPipelineStageFlags2 src_stage_mask,
    VkPipelineStageFlags2 dst_stage_mask
) {
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
        .image = swapchain_images[image_idx],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    const VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = NULL,
        //.dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

	vkCmdPipelineBarrier2(cmd_buf, &dependency_info);
}


void record_cmd_buf(
    Context *ctx,
    VkCommandBuffer cmd_buf,
    uint32_t image_idx
) {
    // begin recording
    VkCommandBufferBeginInfo cmd_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };

    if (vkBeginCommandBuffer(cmd_buf, &cmd_begin_info) != VK_SUCCESS) {
        lv_fatal("Failed to begin recording command buffer.");
    }

    // Transition image layout for rendering
    transition_image_layout(
        cmd_buf,
        image_idx,
        (VkImage *)(ctx->swapchain.images.data),
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_ACCESS_2_NONE,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    );

    VkClearValue clear_color = {
        .color = (VkClearColorValue){1.0f, 0.0f, 0.0f, 1.0f},
        .depthStencil = 0
    };

    VkRenderingAttachmentInfo attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = NULL,
        .imageView = LV_ARRAY_AT(&ctx->swapchain.views, image_idx, VkImageView),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .clearValue = clear_color,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = NULL,
        .flags = 0,
        .renderArea.offset = (VkOffset2D){0, 0},
        .renderArea.extent = ctx->swapchain.extent,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_info
    };

    vkCmdBeginRendering(cmd_buf, &rendering_info);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphics_pipeline);
    vkCmdDraw(cmd_buf, 3, 1, 0, 0);

    vkCmdEndRendering(cmd_buf);

    // Transition image layout for presentation
    transition_image_layout(
        cmd_buf,
        image_idx,
        (VkImage *)(ctx->swapchain.images.data),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
    );

    // stop recording
    if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
        lv_fatal("Failed to record command buffer (vkEndCommandBuffer)");
    }
}


int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
	    lv_fatal("SDL initialization error: %s", SDL_GetError());
        exit(EXIT_FAILURE);
	}

    SDL_Vulkan_LoadLibrary(NULL);

    uint32_t window_width = 1280;
    uint32_t window_height = 720;

    SDL_Window *window = SDL_CreateWindow(
        "Vulkan Playground",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN
    );
    if (!window) {
        lv_fatal("Window creation failed: %s", SDL_GetError());
    }

    Context ctx = {
        .inst = VK_NULL_HANDLE,
        .phydevice = VK_NULL_HANDLE,
        .surface = VK_NULL_HANDLE,
        .families = {INVALID_UINT32_IDX},
        .device = VK_NULL_HANDLE,
        .graphics_q = VK_NULL_HANDLE,
        .present_q = VK_NULL_HANDLE
    };

    if (create_instance(&ctx, window) != 0) {
        lv_fatal("Failed to create instance.");
    }

    if (get_physical_device(&ctx) != 0) {
        lv_fatal("Could not found a GPU on the system with Vulkan support.");
    }

    if (create_logical_device(&ctx) != 0) {
        lv_fatal("Failed to create logical device.");
    }

    if (create_swapchain(&ctx) != 0) {
        lv_fatal("Failed to create swapchain.");
    }

    if (create_graphics_pipeline(&ctx) != 0) {
        lv_fatal("Failed to create graphics pipeline.");
    }


    // COMMAND BUFFERS & POOLS

    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx.families.graphics_idx
    };
    if (vkCreateCommandPool(ctx.device, &cmd_pool_info, NULL, &cmd_pool) != VK_SUCCESS) {
        lv_fatal("Failed to create graphics command pool.");
    }

    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(ctx.device, &alloc_info, &cmd_buf) != VK_SUCCESS) {
        lv_fatal("Failed to allocate command buffer.");
    }


    // SYNCHRONIZATION

    VkSemaphore sem_img_available = VK_NULL_HANDLE;
    VkSemaphore sem_render_finished = VK_NULL_HANDLE;
    VkFence fen_in_flight = VK_NULL_HANDLE;

    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    VkFenceCreateInfo fen_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if (
        vkCreateSemaphore(ctx.device, &sem_info, NULL, &sem_img_available) != VK_SUCCESS ||
        vkCreateSemaphore(ctx.device, &sem_info, NULL, &sem_render_finished) != VK_SUCCESS ||
        vkCreateFence(ctx.device, &fen_info, NULL, &fen_in_flight) != VK_SUCCESS
    ) {
        lv_fatal("Failed to create synchronization structures.");
    }


    lvClock clock = lvClock_new();

    bool is_running = true;
    while (is_running) {
        lvClock_tick(&clock, 60);

        double dt = lvClock_get_delta_time(&clock);
        double fps = lvClock_get_fps(&clock);

        char title[64];
        sprintf(title, "Vulkan Playground - FPS: %.1f", fps);
        SDL_SetWindowTitle(window, title);

        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                is_running = false;
            }

            else if (
                event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED
            ) {
                window_width = event.window.data1;
                window_height = event.window.data2;
            }

            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                    is_running = false;
                }
            }
        }


        // DRAW FRAME

        vkWaitForFences(ctx.device, 1, &fen_in_flight, VK_TRUE, UINT64_MAX);
        vkResetFences(ctx.device, 1, &fen_in_flight);

        uint32_t image_idx = 0;
        if (vkAcquireNextImageKHR(ctx.device, ctx.swapchain.swapchain, UINT64_MAX, sem_img_available, VK_NULL_HANDLE, &image_idx) != VK_SUCCESS) {
            printf("Failed to acquire next image from swapchain, continuing.");
        }

        vkResetCommandBuffer(cmd_buf, 0);
        record_cmd_buf(
            &ctx,
            cmd_buf,
            image_idx
        );

        VkPipelineStageFlags wait_stages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &sem_img_available,
            .pWaitDstStageMask = wait_stages,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &sem_render_finished,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buf
        };

        if (vkQueueSubmit(ctx.graphics_q, 1, &submit_info, fen_in_flight) != VK_SUCCESS) {
            lv_fatal("Failed to submit draw command buffer.");
        }


        // PRESENT FRAME

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = NULL,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &sem_render_finished,
            .swapchainCount = 1,
            .pSwapchains = &ctx.swapchain.swapchain,
            .pImageIndices = &image_idx,
            .pResults = NULL
        };

        if (vkQueuePresentKHR(ctx.present_q, &present_info) != VK_SUCCESS) {
            printf("Failed to present image to swapchain, continuing.");
        }
    }

    // Wait for synchronization to be done before cleanup
    vkDeviceWaitIdle(ctx.device);

    vkDestroyFence(ctx.device, fen_in_flight, NULL);
    vkDestroySemaphore(ctx.device, sem_render_finished, NULL);
    vkDestroySemaphore(ctx.device, sem_img_available, NULL);

    vkDestroyCommandPool(ctx.device, cmd_pool, NULL);

    vkDestroyPipeline(ctx.device, ctx.graphics_pipeline, NULL);

    vkDestroyPipelineLayout(ctx.device, ctx.pipeline_lyt, NULL);

    for (uint32_t i = 0; i < ctx.swapchain.views.size; i++) {
        vkDestroyImageView(ctx.device, LV_ARRAY_AT(&ctx.swapchain.views, i, VkImageView), NULL);
    }
    lvArray_free(&ctx.swapchain.images);
    lvArray_free(&ctx.swapchain.views);
    
    vkDestroySwapchainKHR(ctx.device, ctx.swapchain.swapchain, NULL);
    vkDestroyDevice(ctx.device, NULL);
    vkDestroySurfaceKHR(ctx.inst, ctx.surface, NULL);
    vkDestroyInstance(ctx.inst, NULL);
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    
    printf("Exited with SDL_GetError: '%s'\n", SDL_GetError());

    return EXIT_SUCCESS;
}
