#include <SDL.h>
#include "vk_mem_alloc.h"

#include "lava/internal.h"
#include "lava/vk/context.h"
#include "lava/vk/swapchain.h"


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
static inline int check_layers(
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

/**
 * @brief Create a Vulkan instance.
 * 
 * @param ctx Pointer to lvContext.
 * @param window Pointer to SDL_Window.
 * @return `0` if succesful.
 *         `1` if instance creation failed.
 *         `2` if requested validation layers are not available.
 *         `3` if surface creation failed.
 */
static int create_instance(lvContext *ctx, SDL_Window *window) {
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

    uint32_t n_requested_layers = ctx->_creation.requested_layers.size;
    char **requested_layers = (char **)ctx->_creation.requested_layers.data;

    if (check_layers((const char *const *)requested_layers, n_requested_layers, layer_names, n_layers)) {
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

static inline void physical_device_type_as_str(char *buffer, VkPhysicalDeviceType type) {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            sprintf(buffer, "CPU");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            sprintf(buffer, "Virtual");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            sprintf(buffer, "Integrated");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            sprintf(buffer, "Discrete");
            break;
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            sprintf(buffer, "Other");
            break;
        default:
            sprintf(buffer, "Unknown");
            break;
    }
}

/**
 * @brief Fetch the physical device.
 * 
 * @param ctx Pointer to Context
 * @return `0` if succesful.
 *         `1` if no physical devices are found.
 */
int find_physical_device(lvContext *ctx) {
    uint32_t n_phydevices = 0;
    vkEnumeratePhysicalDevices(ctx->inst, &n_phydevices, NULL);

    if (n_phydevices == 0) {
        return 1;
    }

    VkPhysicalDevice *phydevices = LV_MALLOC(sizeof(VkPhysicalDevice) * n_phydevices);
    vkEnumeratePhysicalDevices(ctx->inst, &n_phydevices, phydevices);

    printf("%u available physical device(s) on the system:\n", n_phydevices);

    for (uint32_t i = 0; i < n_phydevices; i++) {
        VkPhysicalDeviceProperties phydevice_info;
        vkGetPhysicalDeviceProperties(phydevices[i], &phydevice_info);

        char device_type_str[16];
        physical_device_type_as_str(device_type_str, phydevice_info.deviceType);

        uint32_t vk_variant = VK_API_VERSION_VARIANT(phydevice_info.apiVersion);
        uint32_t vk_major = VK_API_VERSION_MAJOR(phydevice_info.apiVersion);
        uint32_t vk_minor = VK_API_VERSION_MINOR(phydevice_info.apiVersion);
        uint32_t vk_patch = VK_API_VERSION_PATCH(phydevice_info.apiVersion);

        printf(
            "  Physical device #%u:\n"
            "  - Name:   %s\n"
            "  - Type:   %s\n"
            "  - Vulkan: %u.%u.%u (variant=%u)\n"
            "\n",
            i,
            phydevice_info.deviceName,
            device_type_str,
            vk_major, vk_minor, vk_patch, vk_variant
        );
    }

    uint32_t discrete_idx = LV_INVALID_INDEX_U32;
    for (uint32_t i = 0; i < n_phydevices; i++) {
        VkPhysicalDeviceProperties phydevice_info;
        vkGetPhysicalDeviceProperties(phydevices[i], &phydevice_info);

        if (phydevice_info.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            discrete_idx = i;
            break;
        }
    }

    // Try to choose the discrete GPU, if doesn't exist, fallback to the first one.
    if (discrete_idx == LV_INVALID_INDEX_U32) {
        ctx->phydevice = phydevices[discrete_idx];
    }
    else {
        ctx->phydevice = phydevices[0];
    }

    LV_FREE(phydevices);

    VkPhysicalDeviceProperties phydevice_info;
    vkGetPhysicalDeviceProperties(ctx->phydevice, &phydevice_info);

    char device_type_str[16];
    physical_device_type_as_str(device_type_str, phydevice_info.deviceType);

    uint32_t vk_variant = VK_API_VERSION_VARIANT(phydevice_info.apiVersion);
    uint32_t vk_major = VK_API_VERSION_MAJOR(phydevice_info.apiVersion);
    uint32_t vk_minor = VK_API_VERSION_MINOR(phydevice_info.apiVersion);
    uint32_t vk_patch = VK_API_VERSION_PATCH(phydevice_info.apiVersion);

    printf(
        "Chosen physical device:\n"
        "- Name:   %s\n"
        "- Type:   %s\n"
        "- Vulkan: %u.%u.%u (variant=%u)\n"
        "- Max 2D image dimension:        %u\n"
        "- Max framebuffer color samples: %u\n"
        "- Max framebuffer resolution:    %ux%u\n"
        "- Max vertex input attributes:   %u\n"
        "- Max memory allocations:        %u\n"
        "- Max sampler anisotropy:        %.1f\n"
        "- Max bound descriptor sets:     %u\n"
        "- Max descriptor sets (uniform): %u\n"
        "- Max descriptor sets (sampler): %u\n"
        "\n",
        phydevice_info.deviceName,
        device_type_str,
        vk_major, vk_minor, vk_patch, vk_variant,
        phydevice_info.limits.maxImageDimension2D,
        (uint32_t)phydevice_info.limits.framebufferColorSampleCounts,
        phydevice_info.limits.maxFramebufferWidth,
        phydevice_info.limits.maxFramebufferHeight,
        phydevice_info.limits.maxVertexInputAttributes,
        phydevice_info.limits.maxMemoryAllocationCount,
        phydevice_info.limits.maxSamplerAnisotropy,
        phydevice_info.limits.maxBoundDescriptorSets,
        phydevice_info.limits.maxDescriptorSetUniformBuffers,
        phydevice_info.limits.maxDescriptorSetSamplers
    );

    return 0;
}

static lvQueueFamilies get_queue_families(VkPhysicalDevice phydevice, VkSurfaceKHR surface) {
    lvQueueFamilies family_indices = {
        .graphics_idx = LV_INVALID_INDEX_U32,
        .present_idx = LV_INVALID_INDEX_U32,
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

// TODO: Use lvArray of extensions
static int validate_physical_device(
    VkPhysicalDevice phydevice,
    const char **requested_extensions,
    uint32_t n_requested_extensions
) {
    uint32_t n_extensions = 0;
    vkEnumerateDeviceExtensionProperties(phydevice, NULL, &n_extensions, NULL);

    // TODO: Better error & memory handling
    if (n_extensions == 0) {
        printf("No extension is supported for this physical device.");
        return 1;
    }

    VkExtensionProperties *available_extensions = LV_MALLOC(sizeof(VkExtensionProperties) * n_extensions);
    vkEnumerateDeviceExtensionProperties(phydevice, NULL, &n_extensions, available_extensions);

    // ENABLED FEATURES:
    // Dynamic rendering
    // Synchronization2
    // Sampler anisotropy

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = NULL,
        .dynamicRendering = VK_FALSE
    };

    VkPhysicalDeviceSynchronization2Features sync2_feat = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynamic_rendering,
        .synchronization2 = VK_FALSE
    };

    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &sync2_feat,
        .features.samplerAnisotropy = VK_FALSE
    };
    vkGetPhysicalDeviceFeatures2(phydevice, &features);

    printf(
        "GPU supports dynamic rendering? %u\n"
        "GPU supports Synchronization2? %u\n"
        "GPU supports sampler anisotropy? %u\n"
        "\n",
        dynamic_rendering.dynamicRendering,
        sync2_feat.synchronization2,
        features.features.samplerAnisotropy
    );

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
            printf("Requested extensions are not supported on this physical device.");
            LV_FREE(available_extensions);
            return 1;
        }
    }

    LV_FREE(available_extensions);
    return 0;
}

/**
 * @brief Create a logical device.
 * 
 * @param ctx Pointer to lvContext.
 * @return `0` if succesful.
 *         `1` if failed to create logical device.
 *         `2` if physical device doesn't meet requirements.
 *         `3` if queue families doesn't meet requirements.
 */
static int create_logical_device(lvContext *ctx) {
    #define N_REQUESTED_DEVICE_EXTENSIONS 6
    const char *requested_device_extensions[N_REQUESTED_DEVICE_EXTENSIONS] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

        VK_KHR_RAY_QUERY_EXTENSION_NAME,
    };

    // Validate the GPU and necessary queue families before creating logical device
    if (validate_physical_device(ctx->phydevice, requested_device_extensions, N_REQUESTED_DEVICE_EXTENSIONS) != 0) {
        return 2;
    }
    ctx->families = get_queue_families(ctx->phydevice, ctx->surface);

    if (
        ctx->families.graphics_idx == LV_INVALID_INDEX_U32 &&
        ctx->families.present_idx == LV_INVALID_INDEX_U32
    ) {
        printf("Requested queues are not found in the physical device.");
        return 3;
    }

    #define N_UNIQUE_FAMILIES 2

    uint32_t unique_families[N_UNIQUE_FAMILIES] = {
        ctx->families.graphics_idx,
        ctx->families.present_idx
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

    VkPhysicalDeviceRayQueryFeaturesKHR ray_query = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
        .pNext = NULL,
        .rayQuery = VK_TRUE
    };

    VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = &ray_query,
        .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceSynchronization2Features sync2_feat = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = &dynamic_rendering,
        .synchronization2 = VK_TRUE,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .bufferDeviceAddress = VK_TRUE,
        .pNext = &sync2_feat
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .accelerationStructure = VK_TRUE,
        .pNext = &buffer_device_address,
    };

    VkPhysicalDeviceFeatures device_features = {
        .samplerAnisotropy = VK_TRUE
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &acceleration_structure,
        .queueCreateInfoCount = N_UNIQUE_FAMILIES,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &device_features,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = N_REQUESTED_DEVICE_EXTENSIONS,
        .ppEnabledExtensionNames = requested_device_extensions,
    };

    if (vkCreateDevice(ctx->phydevice, &device_create_info, NULL, &ctx->device) != VK_SUCCESS) {
        printf("Failed to create logical device.");
        return 1;
    }

    vkGetDeviceQueue(ctx->device, ctx->families.graphics_idx, 0, &ctx->graphics_q);
    vkGetDeviceQueue(ctx->device, ctx->families.present_idx, 0, &ctx->present_q);

    return 0;
}

static void load_extension_prototypes(lvContext *ctx) {
    ctx->ext.vkCreateAccelerationStructureKHR =
    (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(ctx->device, "vkCreateAccelerationStructureKHR");

    ctx->ext.vkDestroyAccelerationStructureKHR =
    (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(ctx->device, "vkDestroyAccelerationStructureKHR");

    ctx->ext.vkGetAccelerationStructureBuildSizesKHR =
    (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(ctx->device, "vkGetAccelerationStructureBuildSizesKHR");

    ctx->ext.vkCmdBuildAccelerationStructuresKHR =
    (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(ctx->device, "vkCmdBuildAccelerationStructuresKHR");

    ctx->ext.vkGetAccelerationStructureDeviceAddressKHR =
    (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(ctx->device, "vkGetAccelerationStructureDeviceAddressKHR");
} 

void lvContext_request_validation_layer(lvContext *ctx, const char *layer_name) {
    // Ensure the request queue is initialized
    if (ctx->_creation.requested_layers.capacity == 0) {
        ctx->_creation.requested_layers = lvRefArray_new();
    }
    
    lvRefArray_add(&ctx->_creation.requested_layers, (void *)layer_name);
}

int lvContext_init(lvContext *ctx, SDL_Window *window) {
    if (create_instance(ctx, window) != 0) {
        printf("Failed to create instance.");
        return 1;
    }

    if (find_physical_device(ctx) != 0) {
        printf("Could not found a GPU on the system with Vulkan support.");
        return 1;
    }

    if (create_logical_device(ctx) != 0) {
        printf("Failed to create logical device.");
        return 1;
    }

    VmaAllocatorCreateInfo allocator_info = {
        .physicalDevice = ctx->phydevice,
        .device = ctx->device,
        .instance = ctx->inst,
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
        // TODO: .vulkanApiVersion = 
    };

    if (vmaCreateAllocator(&allocator_info, &ctx->allocator) != VK_SUCCESS) {
        printf("Failed to create VMA allocator.");
        return 1;
    }

    ctx->swapchains = lvRefArray_new();

    ctx->frame_lag = LV_CONTEXT_DEFAULT_FRAME_LAG;
    ctx->frame_idx = 0;

    // Clean up creation info
    if (ctx->_creation.requested_layers.capacity != 0) {
        lvRefArray_free(&ctx->_creation.requested_layers);
    }

    load_extension_prototypes(ctx);

    return 0;
}

void lvContext_free(lvContext *ctx) {
    if (!ctx) return;

    for (size_t i = 0; i < ctx->swapchains.size; i++) {
        lvSwapchain_free(ctx->swapchains.data[i], ctx);
    }
    lvRefArray_free(&ctx->swapchains);

    vmaDestroyAllocator(ctx->allocator);
    vkDestroyDevice(ctx->device, NULL);
    vkDestroySurfaceKHR(ctx->inst, ctx->surface, NULL);
    vkDestroyInstance(ctx->inst, NULL);
}