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
int find_physical_device(lvContext *ctx) {
    uint32_t n_phydevices = 0;
    vkEnumeratePhysicalDevices(ctx->inst, &n_phydevices, NULL);

    if (n_phydevices == 0) {
        return 1;
    }

    VkPhysicalDevice *phydevices = LV_MALLOC(sizeof(VkPhysicalDevice) * n_phydevices);
    vkEnumeratePhysicalDevices(ctx->inst, &n_phydevices, phydevices);

    uint32_t discrete_idx = LV_INVALID_INDEX_U32;
    for (uint32_t i = 0; i < n_phydevices; i++) {
        VkPhysicalDeviceProperties phydevice_info;
        vkGetPhysicalDeviceProperties(phydevices[i], &phydevice_info);

        if (phydevice_info.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            discrete_idx = i;
            break;
        }
    }

    // Try to choose the discrete GPU, if doesn't exist, fallback to the first one
    if (discrete_idx == LV_INVALID_INDEX_U32) {
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
            printf("Requested extensions are not supported on this physical device.");
            return 1;
        }
    }

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
    #define N_REQUESTED_DEVICE_EXTENSIONS 3
    const char *requested_device_extensions[N_REQUESTED_DEVICE_EXTENSIONS] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
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

    vkGetDeviceQueue(ctx->device, ctx->families.graphics_idx, 0, &ctx->graphics_q);
    vkGetDeviceQueue(ctx->device, ctx->families.present_idx, 0, &ctx->present_q);

    return 0;
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
        // TODO: .vulkanApiVersion = 
    };

    if (vmaCreateAllocator(&allocator_info, &ctx->allocator) != VK_SUCCESS) {
        printf("Failed to create VMA allocator.");
        return 1;
    }

    ctx->swapchains = lvRefArray_new();

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