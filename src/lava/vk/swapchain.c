#include "lava/vk/swapchain.h"
#include "lava/containers/refarray.h"


static lvSwapChainSupport get_swap_chain_support(
    VkPhysicalDevice phydevice,
    VkSurfaceKHR surface
) {
    lvSwapChainSupport sc;
    sc.formats = lvArray_new(sizeof(VkSurfaceFormatKHR));
    sc.present_modes = lvArray_new(sizeof(VkPresentModeKHR));

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        phydevice, surface, &sc.capabilities
    );

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        phydevice, surface, (uint32_t *)&sc.formats.size, NULL
    );

    // TODO: Handle formats.size == 0 and present_modes.sizes == 0
    //       does this case even exist? A GPU without none of those?

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

    lvArray_resize(&sc.present_modes);

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        phydevice,
        surface,
        (uint32_t *)&sc.present_modes.size,
        (VkPresentModeKHR *)sc.present_modes.data
    );

    return sc;
}

int lvSwapchain_init(
    lvSwapchain *swapchain,
    lvContext *ctx,
    uint32_t preferred_extent_width,
    uint32_t preferred_extent_height
) {
    lvSwapChainSupport sc = get_swap_chain_support(ctx->phydevice, ctx->surface);

    // TODO: Simpler enums to choose color spaces, formats and VSYNC mode before creating SC
    // TODO: How does multiple swapchains work? Can there even be multiple?
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

    if (best_extent.width == UINT32_MAX || best_extent.height == UINT32_MAX) {
        // TODO: Clamp between device limits
        best_extent.width = preferred_extent_width;
        best_extent.height = preferred_extent_height;
    }

    uint32_t n_swap_images = sc.capabilities.minImageCount + 1;
    if (sc.capabilities.maxImageCount > 0 && n_swap_images > sc.capabilities.maxImageCount) {
        n_swap_images = sc.capabilities.maxImageCount;
    }

    swapchain->extent = best_extent;
    swapchain->format = best_format;
    swapchain->present_mode = best_present_mode;

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

    if (vkCreateSwapchainKHR(ctx->device, &swapchain_create_info, NULL, &swapchain->swapchain) != VK_SUCCESS) {
        printf("Failed to create swapchain.");
        return 1;
    }

    // IMAGES

    swapchain->images = lvArray_new(sizeof(VkImage));

    vkGetSwapchainImagesKHR(ctx->device, swapchain->swapchain, (uint32_t *)&swapchain->images.size, NULL);

    if (swapchain->images.size == 0) {
        printf("0 swap chain images?");
        return 2;
    }

    lvArray_resize(&swapchain->images);
    vkGetSwapchainImagesKHR(ctx->device, swapchain->swapchain, (uint32_t *)&swapchain->images.size, (VkImage *)(swapchain->images.data));

    printf(
        "Initialized swapchain:\n"
        "- Minimum images: %u\n"
        "- Allocated images: %zu\n"
        "- Extent: %ux%u\n"
        "\n",
        n_swap_images,
        swapchain->images.size,
        best_extent.width, best_extent.height
    );

    // IMAGE VIEWS

    swapchain->views = lvArray_new(sizeof(VkImageView));

    for (uint32_t i = 0; i < swapchain->images.size; i++) {
        VkImageView view = VK_NULL_HANDLE;
        lvArray_add(&swapchain->views, (void *)&view);

        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = LV_ARRAY_AT(&swapchain->images, i, VkImage),
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

        if (vkCreateImageView(ctx->device, &create_info, NULL, LV_ARRAY_PTR_AT(&swapchain->views, i, VkImageView)) != VK_SUCCESS) {
            printf("Failed to create image view.");
            return 3;
        }
    }

    lvArray_free(&sc.formats);
    lvArray_free(&sc.present_modes);

    // SYNCHRONIZATION STRUCTURES

    swapchain->sem_image = lvArray_new(sizeof(VkSemaphore));
    swapchain->sem_present = lvArray_new(sizeof(VkSemaphore));
    swapchain->fen_frame = lvArray_new(sizeof(VkFence));

    swapchain->sem_image.size = ctx->frame_lag;
    lvArray_resize(&swapchain->sem_image);

    swapchain->sem_present.size = swapchain->images.size;
    lvArray_resize(&swapchain->sem_present);

    swapchain->fen_frame.size = ctx->frame_lag;
    lvArray_resize(&swapchain->fen_frame);

    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    // Start the fence as signaled so the main application loop can start
    VkFenceCreateInfo fen_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (size_t i = 0; i < ctx->frame_lag; i++) {
        if (
            vkCreateSemaphore(ctx->device, &sem_info, NULL, LV_ARRAY_PTR_AT(&swapchain->sem_image, i, VkSemaphore)) != VK_SUCCESS ||
            vkCreateFence(ctx->device, &fen_info, NULL, LV_ARRAY_PTR_AT(&swapchain->fen_frame, i, VkFence)) != VK_SUCCESS
        ) {
            printf("Failed to create synchronization structures.");
            return 4;
        }
    }

    for (size_t i = 0; i < swapchain->sem_present.size; i++) {
        if (vkCreateSemaphore(ctx->device, &sem_info, NULL, LV_ARRAY_PTR_AT(&swapchain->sem_present, i, VkSemaphore)) != VK_SUCCESS) {
            printf("Failed to create 'render_finished' semaphore.");
            return 4;
        }
    }

    printf(
        "Swapchain synchronization:\n"
        "- Frame lag:          %zu\n"
        "- Fences:             %zu\n"
        "- Image semaphores:   %zu\n"
        "- Present semaphores: %zu\n"
        "\n",
        ctx->frame_lag,
        swapchain->fen_frame.size,
        swapchain->sem_image.size,
        swapchain->fen_frame.size
    );

    return lvRefArray_add(&ctx->swapchains, swapchain);
}

void lvSwapchain_free(lvSwapchain *swapchain, lvContext *ctx) {
    if (!swapchain) return;

    for (size_t i = 0; i < swapchain->sem_image.size; i++) {
        vkDestroyFence(ctx->device, LV_ARRAY_AT(&swapchain->fen_frame, i, VkFence), NULL);
        vkDestroySemaphore(ctx->device, LV_ARRAY_AT(&swapchain->sem_image, i, VkSemaphore), NULL);
    }
    for (size_t i = 0; i < swapchain->sem_present.size; i++) {
        vkDestroySemaphore(ctx->device, LV_ARRAY_AT(&swapchain->sem_present, i, VkSemaphore), NULL);
    }
    lvArray_free(&swapchain->sem_image);
    lvArray_free(&swapchain->sem_present);
    lvArray_free(&swapchain->fen_frame);

    for (uint32_t i = 0; i < swapchain->views.size; i++) {
        vkDestroyImageView(ctx->device, LV_ARRAY_AT(&swapchain->views, i, VkImageView), NULL);
    }
    lvArray_free(&swapchain->images);
    lvArray_free(&swapchain->views);
    
    vkDestroySwapchainKHR(ctx->device, swapchain->swapchain, NULL);
}