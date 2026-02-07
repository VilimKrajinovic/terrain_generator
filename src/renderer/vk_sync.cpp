#include "vk_sync.h"
#include "core/log.h"
#include <string.h>

VkResult vk_sync_create(VkDevice device, VkSyncContext *ctx) {
    LOG_INFO("Creating sync objects");

    memset(ctx, 0, sizeof(VkSyncContext));
    ctx->current_frame = 0;

    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,  // Start signaled
    };

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result;

        result = vkCreateSemaphore(device, &semaphore_info, NULL, &ctx->frames[i].image_available);
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create image available semaphore %u: %d", i, result);
            return result;
        }

        result = vkCreateSemaphore(device, &semaphore_info, NULL, &ctx->frames[i].render_finished);
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create render finished semaphore %u: %d", i, result);
            return result;
        }

        result = vkCreateFence(device, &fence_info, NULL, &ctx->frames[i].in_flight);
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create fence %u: %d", i, result);
            return result;
        }
    }

    LOG_INFO("Sync objects created for %d frames in flight", MAX_FRAMES_IN_FLIGHT);
    return VK_SUCCESS;
}

void vk_sync_destroy(VkDevice device, VkSyncContext *ctx) {
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (ctx->frames[i].image_available != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, ctx->frames[i].image_available, NULL);
        }
        if (ctx->frames[i].render_finished != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, ctx->frames[i].render_finished, NULL);
        }
        if (ctx->frames[i].in_flight != VK_NULL_HANDLE) {
            vkDestroyFence(device, ctx->frames[i].in_flight, NULL);
        }
    }

    LOG_DEBUG("Sync objects destroyed");
    memset(ctx, 0, sizeof(VkSyncContext));
}

VkResult vk_sync_wait_for_fence(VkDevice device, VkSyncContext *ctx) {
    return vkWaitForFences(device, 1, &ctx->frames[ctx->current_frame].in_flight, VK_TRUE, UINT64_MAX);
}

VkResult vk_sync_reset_fence(VkDevice device, VkSyncContext *ctx) {
    return vkResetFences(device, 1, &ctx->frames[ctx->current_frame].in_flight);
}

FrameSync *vk_sync_get_current_frame(VkSyncContext *ctx) {
    return &ctx->frames[ctx->current_frame];
}

void vk_sync_advance_frame(VkSyncContext *ctx) {
    ctx->current_frame = (ctx->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
