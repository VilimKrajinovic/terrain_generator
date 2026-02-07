#include "vk_command.h"
#include "core/log.h"
#include <string.h>

VkResult vk_command_create(VkDevice device, u32 queue_family_index, VkCommandContext *ctx) {
    LOG_INFO("Creating command pool and buffers");

    memset(ctx, 0, sizeof(VkCommandContext));

    // Create command pool
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_index,
    };

    VkResult result = vkCreateCommandPool(device, &pool_info, NULL, &ctx->pool);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create command pool: %d", result);
        return result;
    }

    // Allocate command buffers
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ctx->pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
    };

    result = vkAllocateCommandBuffers(device, &alloc_info, ctx->buffers);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate command buffers: %d", result);
        vkDestroyCommandPool(device, ctx->pool, NULL);
        return result;
    }

    LOG_INFO("Command pool and %d buffers created", MAX_FRAMES_IN_FLIGHT);
    return VK_SUCCESS;
}

void vk_command_destroy(VkDevice device, VkCommandContext *ctx) {
    if (ctx->pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, ctx->pool, NULL);
    }

    LOG_DEBUG("Command pool destroyed");
    memset(ctx, 0, sizeof(VkCommandContext));
}

VkCommandBuffer vk_command_get_buffer(VkCommandContext *ctx, u32 frame_index) {
    return ctx->buffers[frame_index];
}

VkResult vk_command_begin(VkCommandBuffer buffer) {
    vkResetCommandBuffer(buffer, 0);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = NULL,
    };

    return vkBeginCommandBuffer(buffer, &begin_info);
}

VkResult vk_command_end(VkCommandBuffer buffer) {
    return vkEndCommandBuffer(buffer);
}

VkCommandBuffer vk_command_begin_single(VkDevice device, VkCommandPool pool) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer buffer;
    if (vkAllocateCommandBuffers(device, &alloc_info, &buffer) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(buffer, &begin_info);
    return buffer;
}

VkResult vk_command_end_single(VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer buffer) {
    vkEndCommandBuffer(buffer);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &buffer,
    };

    VkResult result = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, &buffer);
    return result;
}
