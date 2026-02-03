#include "vk_swapchain.h"
#include "core/log.h"
#include "utils/macros.h"
#include "memory/arena.h"
#include <string.h>

SwapchainSupportDetails vk_swapchain_query_support(VkPhysicalDevice device, VkSurfaceKHR surface, Arena *arena) {
    SwapchainSupportDetails details = {0};

    // Get capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // Get formats
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, NULL);
    if (details.format_count > 0) {
        details.formats = ARENA_PUSH_ARRAY(arena, VkSurfaceFormatKHR, details.format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, details.formats);
    }

    // Get present modes
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, NULL);
    if (details.present_mode_count > 0) {
        details.present_modes = ARENA_PUSH_ARRAY(arena, VkPresentModeKHR, details.present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, details.present_modes);
    }

    return details;
}

// Choose surface format
static VkSurfaceFormatKHR choose_surface_format(const VkSurfaceFormatKHR *formats, u32 count) {
    // Prefer SRGB with B8G8R8A8 format
    for (u32 i = 0; i < count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }

    // Fall back to first available
    return formats[0];
}

// Choose present mode
static VkPresentModeKHR choose_present_mode(const VkPresentModeKHR *modes, u32 count) {
    // Prefer mailbox (triple buffering)
    for (u32 i = 0; i < count; i++) {
        if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return modes[i];
        }
    }

    // FIFO is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Choose swap extent
static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR *capabilities, u32 width, u32 height) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }

    VkExtent2D extent = { width, height };

    extent.width = CLAMP(extent.width,
                         capabilities->minImageExtent.width,
                         capabilities->maxImageExtent.width);
    extent.height = CLAMP(extent.height,
                          capabilities->minImageExtent.height,
                          capabilities->maxImageExtent.height);

    return extent;
}

// Create image views
static VkResult create_image_views(VkDevice device, VkSwapchainContext *ctx) {
    for (u32 i = 0; i < ctx->image_count; i++) {
        VkImageViewCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = ctx->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = ctx->format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        VkResult result = vkCreateImageView(device, &create_info, NULL, &ctx->image_views[i]);
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create image view %u: %d", i, result);
            return result;
        }
    }

    return VK_SUCCESS;
}

VkResult vk_swapchain_create(VkDeviceContext *device, VkSurfaceKHR surface,
                             u32 width, u32 height, VkSwapchainKHR old_swapchain,
                             VkSwapchainContext *ctx) {
    LOG_INFO("Creating swapchain (%ux%u)", width, height);

    memset(ctx, 0, sizeof(VkSwapchainContext));

    // Query support details using scratch arena
    ArenaTemp scratch = arena_scratch_begin();
    SwapchainSupportDetails support = vk_swapchain_query_support(device->physical_device, surface, scratch.arena);

    VkSurfaceFormatKHR surface_format = choose_surface_format(support.formats, support.format_count);
    VkPresentModeKHR present_mode = choose_present_mode(support.present_modes, support.present_mode_count);
    VkExtent2D extent = choose_extent(&support.capabilities, width, height);

    // Determine image count
    u32 image_count = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount) {
        image_count = support.capabilities.maxImageCount;
    }
    if (image_count > MAX_SWAPCHAIN_IMAGES) {
        image_count = MAX_SWAPCHAIN_IMAGES;
    }

    LOG_DEBUG("Swapchain format: %d, present mode: %d, image count: %u",
              surface_format.format, present_mode, image_count);

    // Create swapchain
    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = support.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };

    // Handle queue family sharing
    u32 queue_family_indices[] = {
        device->queue_families.graphics_family,
        device->queue_families.present_family,
    };

    if (device->queue_families.graphics_family != device->queue_families.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkResult result = vkCreateSwapchainKHR(device->device, &create_info, NULL, &ctx->swapchain);
    arena_temp_end(scratch);  // Free support details

    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create swapchain: %d", result);
        return result;
    }

    // Get swapchain images
    vkGetSwapchainImagesKHR(device->device, ctx->swapchain, &ctx->image_count, NULL);
    if (ctx->image_count > MAX_SWAPCHAIN_IMAGES) {
        ctx->image_count = MAX_SWAPCHAIN_IMAGES;
    }
    vkGetSwapchainImagesKHR(device->device, ctx->swapchain, &ctx->image_count, ctx->images);

    ctx->format = surface_format.format;
    ctx->extent = extent;

    // Create image views
    result = create_image_views(device->device, ctx);
    if (result != VK_SUCCESS) {
        vkDestroySwapchainKHR(device->device, ctx->swapchain, NULL);
        return result;
    }

    LOG_INFO("Swapchain created with %u images", ctx->image_count);
    return VK_SUCCESS;
}

void vk_swapchain_destroy(VkDeviceContext *device, VkSwapchainContext *ctx) {
    // Destroy image views
    for (u32 i = 0; i < ctx->image_count; i++) {
        if (ctx->image_views[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(device->device, ctx->image_views[i], NULL);
        }
    }

    // Destroy swapchain
    if (ctx->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device->device, ctx->swapchain, NULL);
    }

    LOG_DEBUG("Swapchain destroyed");
    memset(ctx, 0, sizeof(VkSwapchainContext));
}

VkResult vk_swapchain_recreate(VkDeviceContext *device, VkSurfaceKHR surface,
                               u32 width, u32 height, VkSwapchainContext *ctx) {
    LOG_INFO("Recreating swapchain");

    VkSwapchainKHR old_swapchain = ctx->swapchain;

    // Destroy old image views
    for (u32 i = 0; i < ctx->image_count; i++) {
        if (ctx->image_views[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(device->device, ctx->image_views[i], NULL);
            ctx->image_views[i] = VK_NULL_HANDLE;
        }
    }

    // Create new swapchain (old swapchain will be used as parameter)
    VkResult result = vk_swapchain_create(device, surface, width, height, old_swapchain, ctx);

    // Destroy old swapchain after new one is created
    if (old_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device->device, old_swapchain, NULL);
    }

    return result;
}
