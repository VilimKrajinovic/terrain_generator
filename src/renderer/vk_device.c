#include "vk_device.h"
#include "core/log.h"
#include "utils/macros.h"
#include "utils/arena.h"
#include <string.h>

// Required device extensions
static const char *DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",  // Required for MoltenVK
};
static const u32 DEVICE_EXTENSION_COUNT = 2;

// Check device extension support
static bool check_device_extension_support(VkPhysicalDevice device) {
    u32 extension_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);

    ArenaTemp scratch = arena_scratch_begin();
    VkExtensionProperties *available_extensions = ARENA_PUSH_ARRAY(scratch.arena, VkExtensionProperties, extension_count);
    if (!available_extensions) {
        arena_temp_end(scratch);
        return false;
    }

    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, available_extensions);

    u32 found_count = 0;
    for (u32 i = 0; i < DEVICE_EXTENSION_COUNT; i++) {
        for (u32 j = 0; j < extension_count; j++) {
            if (strcmp(DEVICE_EXTENSIONS[i], available_extensions[j].extensionName) == 0) {
                found_count++;
                break;
            }
        }
    }

    arena_temp_end(scratch);

    // At minimum, swapchain is required
    return found_count >= 1;
}

QueueFamilyIndices vk_device_find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices = {
        .graphics_family_found = false,
        .present_family_found = false,
    };

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    ArenaTemp scratch = arena_scratch_begin();
    VkQueueFamilyProperties *queue_families = ARENA_PUSH_ARRAY(scratch.arena, VkQueueFamilyProperties, queue_family_count);
    if (!queue_families) {
        arena_temp_end(scratch);
        return indices;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    for (u32 i = 0; i < queue_family_count; i++) {
        // Check graphics support
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            indices.graphics_family_found = true;
        }

        // Check present support
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
        if (present_support) {
            indices.present_family = i;
            indices.present_family_found = true;
        }

        if (indices.graphics_family_found && indices.present_family_found) {
            break;
        }
    }

    arena_temp_end(scratch);
    return indices;
}

bool vk_device_queue_families_complete(const QueueFamilyIndices *indices) {
    return indices->graphics_family_found && indices->present_family_found;
}

// Rate device suitability
static u32 rate_device(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    u32 score = 0;

    // Prefer discrete GPUs
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        score += 100;
    }

    // Max image dimension affects quality
    score += properties.limits.maxImageDimension2D;

    // Check required features
    QueueFamilyIndices indices = vk_device_find_queue_families(device, surface);
    if (!vk_device_queue_families_complete(&indices)) {
        return 0;
    }

    if (!check_device_extension_support(device)) {
        return 0;
    }

    return score;
}

// Select physical device
static VkResult select_physical_device(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDevice *selected) {
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);

    if (device_count == 0) {
        LOG_ERROR("No GPUs with Vulkan support found");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    ArenaTemp scratch = arena_scratch_begin();
    VkPhysicalDevice *devices = ARENA_PUSH_ARRAY(scratch.arena, VkPhysicalDevice, device_count);
    if (!devices) {
        arena_temp_end(scratch);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    // Rate and select best device
    VkPhysicalDevice best_device = VK_NULL_HANDLE;
    u32 best_score = 0;

    LOG_INFO("Found %u physical device(s):", device_count);
    for (u32 i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);

        u32 score = rate_device(devices[i], surface);
        LOG_INFO("  [%u] %s (score: %u)", i, properties.deviceName, score);

        if (score > best_score) {
            best_score = score;
            best_device = devices[i];
        }
    }

    arena_temp_end(scratch);

    if (best_device == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable GPU found");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    *selected = best_device;
    return VK_SUCCESS;
}

VkResult vk_device_create(VkInstance instance, VkSurfaceKHR surface, VkDeviceContext *ctx) {
    LOG_INFO("Creating Vulkan device");

    memset(ctx, 0, sizeof(VkDeviceContext));

    // Select physical device
    VkResult result = select_physical_device(instance, surface, &ctx->physical_device);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Get device properties
    vkGetPhysicalDeviceProperties(ctx->physical_device, &ctx->properties);
    vkGetPhysicalDeviceFeatures(ctx->physical_device, &ctx->features);
    vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &ctx->memory_properties);

    LOG_INFO("Selected device: %s", ctx->properties.deviceName);

    // Find queue families
    ctx->queue_families = vk_device_find_queue_families(ctx->physical_device, surface);

    // Create queue create infos
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_infos[2];
    u32 queue_create_info_count = 0;

    // Graphics queue
    queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = ctx->queue_families.graphics_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    // Present queue (if different family)
    if (ctx->queue_families.present_family != ctx->queue_families.graphics_family) {
        queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = ctx->queue_families.present_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
    }

    // Device features
    VkPhysicalDeviceFeatures device_features = {0};

    // Create logical device
    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = queue_create_info_count,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &device_features,
        .enabledExtensionCount = DEVICE_EXTENSION_COUNT,
        .ppEnabledExtensionNames = DEVICE_EXTENSIONS,
        .enabledLayerCount = 0,
    };

    result = vkCreateDevice(ctx->physical_device, &create_info, NULL, &ctx->device);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device: %d", result);
        return result;
    }

    // Get queue handles
    vkGetDeviceQueue(ctx->device, ctx->queue_families.graphics_family, 0, &ctx->graphics_queue);
    vkGetDeviceQueue(ctx->device, ctx->queue_families.present_family, 0, &ctx->present_queue);

    LOG_INFO("Vulkan device created");
    return VK_SUCCESS;
}

void vk_device_destroy(VkDeviceContext *ctx) {
    if (ctx->device != VK_NULL_HANDLE) {
        vkDestroyDevice(ctx->device, NULL);
        LOG_INFO("Vulkan device destroyed");
    }

    memset(ctx, 0, sizeof(VkDeviceContext));
}

u32 vk_device_find_memory_type(VkDeviceContext *ctx, u32 type_filter, VkMemoryPropertyFlags properties) {
    for (u32 i = 0; i < ctx->memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) &&
            (ctx->memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    LOG_ERROR("Failed to find suitable memory type");
    return UINT32_MAX;
}

void vk_device_wait_idle(VkDeviceContext *ctx) {
    vkDeviceWaitIdle(ctx->device);
}
