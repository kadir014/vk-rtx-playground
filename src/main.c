#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>


/**
 * @brief Log fatal message and exit with status code 1.
 * 
 * @param fmt Formatter string for the message
 * @param ... 
 */
static inline void fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    printf("[FATAL] ");
    vprintf(fmt, args);
    printf("\n");

    va_end(args);

    exit(EXIT_FAILURE);
}


typedef struct {
    size_t length;
    char *data;
} FileContent;

FileContent kt_read_file_raw(const char *filepath) {
    FileContent cont = {.length = 0, .data = NULL};

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        //ns_throw_error("Failed to open file.", 0, nsErrorSeverity_ERROR);
        fatal("Failed to open file at '%s'", filepath);
        return cont;
    }

    // Seek to the end & rewind back to determine the file size
    fseek(file, 0, SEEK_END);
    size_t length = (size_t)ftell(file);
    rewind(file);

    char *buffer = malloc(length + 1);
    if (!buffer) {
        fclose(file);
        //NS_MEM_CHECK(buffer);
        fatal("Failed to allocate memory.");
        return cont;
    }

    fread(buffer, 1, length, file);
    // Make sure to null-terminate the content
    buffer[length] = '\0';

    fclose(file);
    
    cont.length = length;
    cont.data = buffer;
    return cont;
}


/**
 * @brief Check that all requested validation layers are available.
 * 
 * @param requested_layers Array of requested validation layer names.
 * @param n_requested_layers Number of requested validation layer names.
 * @param available_layers Array of currently available validation layer names.
 * @param n_available_layers Number of currently available validation layer names.
 * @return 0 if all requested layers are available.
 *         1 if at least one requested layer is missing.
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


/**
 * @brief Create a Vulkan instance.
 * 
 * Returns non-zero on error.
 * 
 * @param inst Pointer to VkInstance.
 * @param window Pointer to SDL_Window.
 * @return int Error code.
 */
int create_instance(VkInstance *inst, SDL_Window *window) {
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
    extension_names = malloc(sizeof(char *) * n_extensions);;
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
    layer_names = malloc(sizeof(VkLayerProperties) * n_layers);
    vkEnumerateInstanceLayerProperties(&n_layers, layer_names);

    printf("Found %u validation layers:\n", n_layers);
    for (uint32_t i = 0; i < n_layers; i++) {
        printf("%u: %s -- %s\n", i, layer_names[i].layerName, layer_names[i].description);
    }
    printf("\n");

    if (check_layers(requested_layers, n_requested_layers, layer_names, n_layers)) {
        // TODO: nv_set_error
        printf("Requested validation layers are not available on the system.");
        free(extension_names);
        free(layer_names);
        return 2;
    }
    else {
        printf("Applied %u validationSDL_Vulkan_GetInstanceExtensions layers:\n", n_requested_layers);
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

    if (vkCreateInstance(&create_info, NULL, inst) != VK_SUCCESS) {
        ret = 1;
    }

    free(extension_names);
    free(layer_names);

    return ret;
}


typedef struct {
    uint32_t graphics_idx;
    uint32_t present_idx;
} QueueFamilies;

#define INVALID_FAMILY_IDX UINT32_MAX

QueueFamilies find_queue_families(VkPhysicalDevice phydevice, VkSurfaceKHR surface) {
    QueueFamilies family_indices = {
        .graphics_idx = INVALID_FAMILY_IDX,
        .present_idx = INVALID_FAMILY_IDX,
    };

    uint32_t n_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phydevice, &n_families, NULL);

    VkQueueFamilyProperties *families = malloc(sizeof(VkQueueFamilyProperties) * n_families);
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

    free(families);
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
        fatal("No extension is supported for this physical device.");
        return;
    }

    VkExtensionProperties *available_extensions = malloc(sizeof(VkExtensionProperties) * n_extensions);
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
            fatal("Requested extensions are not supported on this physical device.");
            return;
        }
    }
}


// TODO: Use nvValueArrays
typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t n_formats;
    VkPresentModeKHR *present_modes;
    uint32_t n_present_modes;
} SwapChainSupport;

SwapChainSupport get_swap_chain_support(
    VkPhysicalDevice phydevice,
    VkSurfaceKHR surface
) {
    SwapChainSupport sc;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phydevice, surface, &sc.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(phydevice, surface, &sc.n_formats, NULL);

    if (sc.n_formats == 0) {
        fatal("0 formats?");
        return sc;
    }

    sc.formats = malloc(sizeof(VkSurfaceFormatKHR) * sc.n_formats);

    vkGetPhysicalDeviceSurfaceFormatsKHR(phydevice, surface, &sc.n_formats, sc.formats);

    vkGetPhysicalDeviceSurfacePresentModesKHR(phydevice, surface, &sc.n_present_modes, NULL);

    if (sc.n_present_modes == 0) {
        fatal("0 present modes?");
        return sc;
    }

    sc.present_modes = malloc(sizeof(VkPresentModeKHR) * sc.n_present_modes);

    vkGetPhysicalDeviceSurfacePresentModesKHR(phydevice, surface, &sc.n_present_modes, sc.present_modes);

    return sc;
}


VkShaderModule create_shader_module(VkDevice device, const char *filepath) {
    FileContent shader_source = kt_read_file_raw(filepath);

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
        fatal("Failed to create shader module.");
    }

    free(shader_source.data);

    return shader_module;
}


// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/03_Drawing/01_Command_buffers.html#_image_layout_transitions
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
    VkCommandBuffer cmd_buf,
    VkImage *swapchain_images,
    VkImageView *views,
    VkExtent2D render_extent,
    VkPipeline graphics_pipeline,
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
        fatal("Failed to begin recording command buffer.");
    }

    // Transition image layout for rendering
    transition_image_layout(
        cmd_buf,
        image_idx,
        swapchain_images,
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
        .imageView = views[image_idx],
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
        .renderArea.extent = render_extent,
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment_info
    };

    vkCmdBeginRendering(cmd_buf, &rendering_info);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    vkCmdDraw(cmd_buf, 3, 1, 0, 0);

    vkCmdEndRendering(cmd_buf);

    // Transition image layout for presentation
    transition_image_layout(
        cmd_buf,
        image_idx,
        swapchain_images,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT
    );

    // stop recording
    if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
        fatal("Failed to record command buffer (vkEndCommandBuffer)");
    }
}


int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
	    fatal("SDL initialization error: %s", SDL_GetError());
        exit(EXIT_FAILURE);
	}

    SDL_Vulkan_LoadLibrary(NULL);

    uint32_t window_width = 1280;
    uint32_t window_height = 720;

    SDL_Window *window = SDL_CreateWindow(
        "Vulkan RTX Playground",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN
    );
    if (!window) {
        fatal("Window creation failed: %s", SDL_GetError());
    }

    VkInstance inst = VK_NULL_HANDLE;
    if (create_instance(&inst, window)) {
        fatal("Failed to create instance.");
    }

    // WINDOW SURFACE

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!SDL_Vulkan_CreateSurface(window, inst, &surface)) {
        fatal("Failed to create window surface.");
    }


    // PHYSICAL DEVICE

    uint32_t n_phydevices = 0;
    vkEnumeratePhysicalDevices(inst, &n_phydevices, NULL);

    if (n_phydevices == 0) {
        fatal("Could not found GPUs on the system with Vulkan support.");
    }

    VkPhysicalDevice *phydevices = malloc(sizeof(VkPhysicalDevice) * n_phydevices);
    vkEnumeratePhysicalDevices(inst, &n_phydevices, phydevices);

    VkPhysicalDevice phydevice = phydevices[0];
    free(phydevices);

    VkPhysicalDeviceProperties phydevice_info;
    vkGetPhysicalDeviceProperties(phydevice, &phydevice_info);

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


    // LOGICAL DEVICE

    #define N_REQUESTED_DEVICE_EXTENSIONS 3
    const char *requested_device_extensions[N_REQUESTED_DEVICE_EXTENSIONS] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
    };

    validate_physical_device(phydevice, requested_device_extensions, N_REQUESTED_DEVICE_EXTENSIONS);
    
    QueueFamilies families = find_queue_families(phydevice, surface);

    if (
        families.graphics_idx == INVALID_FAMILY_IDX &&
        families.present_idx == INVALID_FAMILY_IDX
    ) {
        fatal("Requested queues are not found in the physical device.");
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

    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(phydevice, &device_create_info, NULL, &device) != VK_SUCCESS) {
        fatal("Failed to create logical device.");
    }

    VkQueue graphics_q = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, families.graphics_idx, 0, &graphics_q);

    VkQueue present_q = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, families.present_idx, 0, &present_q);


    // SWAP CHAIN

    SwapChainSupport sc = get_swap_chain_support(phydevice, surface);

    VkSurfaceFormatKHR best_format = sc.formats[0];
    for (uint32_t i = 0; i < sc.n_formats; i++) {
        VkSurfaceFormatKHR format = sc.formats[i];

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            best_format = format;
            break;
        }
    }

    // MAILBOX = triple buffering
    // FIFO = vsync
    VkPresentModeKHR best_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < sc.n_present_modes; i++) {
        VkPresentModeKHR present_mode = sc.present_modes[i];

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

    VkSwapchainCreateInfoKHR swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .surface = surface,
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

    if (families.graphics_idx != families.present_idx) {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = N_UNIQUE_FAMILIES;
        swapchain_create_info.pQueueFamilyIndices = unique_families;
    }
    else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = NULL;
    }

    free(sc.formats);
    free(sc.present_modes);

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device, &swapchain_create_info, NULL, &swapchain) != VK_SUCCESS) {
        fatal("Failed to create swapchain.");
    }

    VkImage *swapchain_images = NULL;
    uint32_t n_swapchain_images = 0;

    vkGetSwapchainImagesKHR(device, swapchain, &n_swapchain_images, NULL);

    if (n_swapchain_images == 0) {
        fatal("0 swap chain images?");
    }

    swapchain_images = malloc(sizeof(VkImage) * n_swapchain_images);
    vkGetSwapchainImagesKHR(device, swapchain, &n_swapchain_images, swapchain_images);

    printf(
        "Initialized swapchain:\n"
        "- Minimum images: %u\n"
        "- Allocated images: %u\n"
        "- Extent: %ux%u\n"
        "\n",
        n_swap_images,
        n_swapchain_images,
        best_extent.width, best_extent.height
    );


    // IMAGE VIEWS

    VkImageView *swapchain_image_views = malloc(sizeof(VkImageView) * n_swapchain_images);

    for (uint32_t i = 0; i < n_swapchain_images; i++) {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = swapchain_images[i],
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

        if (vkCreateImageView(device, &create_info, NULL, &swapchain_image_views[i]) != VK_SUCCESS) {
            fatal("Failed to create image view.");
        }
    }


    // SHADERS

    VkShaderModule vert_module = create_shader_module(device, "../shaders/first.vert.spv");
    VkShaderModule frag_module = create_shader_module(device, "../shaders/first.frag.spv");


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
        .width = (float)best_extent.width,
        .height = (float)best_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .extent = best_extent,
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

    VkPipelineLayout pipeline_lyt = VK_NULL_HANDLE;

    VkPipelineLayoutCreateInfo pipeline_lyt_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    if (vkCreatePipelineLayout(device, &pipeline_lyt_info, NULL, &pipeline_lyt) != VK_SUCCESS) {
        fatal("Failed to create pipeline layout.");
    }

    
    // GRAPHICS PIPELINE

    VkPipelineRenderingCreateInfo pipeline_rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &best_format.format
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
        .layout = pipeline_lyt,

        // Renderpass
        .renderPass = NULL,
        .subpass = 0,

        // For graphics pipeline derivation
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    VkPipeline graphics_pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &graphics_pipeline) != VK_SUCCESS) {
        fatal("Failed to create graphics pipeline.");
    }


    // COMMAND BUFFERS & POOLS

    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = families.graphics_idx
    };
    if (vkCreateCommandPool(device, &cmd_pool_info, NULL, &cmd_pool) != VK_SUCCESS) {
        fatal("Failed to create graphics command pool.");
    }

    VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(device, &alloc_info, &cmd_buf) != VK_SUCCESS) {
        fatal("Failed to allocate command buffer.");
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
        vkCreateSemaphore(device, &sem_info, NULL, &sem_img_available) != VK_SUCCESS ||
        vkCreateSemaphore(device, &sem_info, NULL, &sem_render_finished) != VK_SUCCESS ||
        vkCreateFence(device, &fen_info, NULL, &fen_in_flight) != VK_SUCCESS
    ) {
        fatal("Failed to create synchronization structures.");
    }



    bool is_running = true;
    while (is_running) {
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

        vkWaitForFences(device, 1, &fen_in_flight, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &fen_in_flight);

        uint32_t image_idx = 0;
        if (vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, sem_img_available, VK_NULL_HANDLE, &image_idx) != VK_SUCCESS) {
            printf("Failed to acquire next image from swapchain, continuing.");
        }

        vkResetCommandBuffer(cmd_buf, 0);
        record_cmd_buf(
            cmd_buf,
            swapchain_images,
            swapchain_image_views,
            best_extent,
            graphics_pipeline,
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

        if (vkQueueSubmit(graphics_q, 1, &submit_info, fen_in_flight) != VK_SUCCESS) {
            fatal("Failed to submit draw command buffer.");
        }


        // PRESENT FRAME

        VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = NULL,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &sem_render_finished,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_idx,
            .pResults = NULL
        };

        if (vkQueuePresentKHR(present_q, &present_info) != VK_SUCCESS) {
            printf("Failed to present image to swapchain, continuing.");
        }
    }

    // Wait for synchronization to be done before cleanup
    vkDeviceWaitIdle(device);

    vkDestroyFence(device, fen_in_flight, NULL);
    vkDestroySemaphore(device, sem_render_finished, NULL);
    vkDestroySemaphore(device, sem_img_available, NULL);

    vkDestroyCommandPool(device, cmd_pool, NULL);

    vkDestroyPipeline(device, graphics_pipeline, NULL);

    vkDestroyPipelineLayout(device, pipeline_lyt, NULL);

    vkDestroyShaderModule(device, frag_module, NULL);
    vkDestroyShaderModule(device, vert_module, NULL);

    for (uint32_t i = 0; i < n_swapchain_images; i++) {
        vkDestroyImageView(device, swapchain_image_views[i], NULL);
    }
    free(swapchain_image_views);
    free(swapchain_images);
    
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(inst, surface, NULL);
    vkDestroyInstance(inst, NULL);
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    
    printf("Exited with SDL_GetError: '%s'\n", SDL_GetError());

    return EXIT_SUCCESS;
}
