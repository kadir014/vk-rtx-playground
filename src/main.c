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
    char **extension_names = NULL;
    SDL_Vulkan_GetInstanceExtensions(window, &n_extensions, NULL);
    extension_names = malloc(sizeof(char *) * n_extensions);;
    SDL_Vulkan_GetInstanceExtensions(window, &n_extensions, extension_names);

    printf("Found %u extensions:\n", n_extensions);
    for (uint32_t i = 0; i < n_extensions; i++) {
        printf("%u: %s\n", i, extension_names[i]);
    }
    printf("\n");

    char *requested_layers[] = {
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
        .graphics_idx = INVALID_FAMILY_IDX
    };

    uint32_t n_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phydevice, &n_families, NULL);

    VkQueueFamilyProperties *families = malloc(sizeof(VkQueueFamilyProperties) * n_families);

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
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN
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

    #define N_REQUESTED_DEVICE_EXTENSIONS 1
    const char *requested_device_extensions[N_REQUESTED_DEVICE_EXTENSIONS] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
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
    }


    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(inst, surface, NULL);
    vkDestroyInstance(inst, NULL);
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();

    return EXIT_SUCCESS;
}