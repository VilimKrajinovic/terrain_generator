#ifndef VK_SYNC_H
#define VK_SYNC_H

#include <vulkan/vulkan.h>
#include "utils/types.h"

// Maximum frames in flight
#define MAX_FRAMES_IN_FLIGHT 2

// Synchronization context for a single frame
typedef struct FrameSync {
    VkSemaphore image_available;
    VkSemaphore render_finished;
    VkFence in_flight;
} FrameSync;

// Sync context
typedef struct VkSyncContext {
    FrameSync frames[MAX_FRAMES_IN_FLIGHT];
    u32 current_frame;
} VkSyncContext;

// Create sync objects
VkResult vk_sync_create(VkDevice device, VkSyncContext *ctx);

// Destroy sync objects
void vk_sync_destroy(VkDevice device, VkSyncContext *ctx);

// Wait for current frame fence
VkResult vk_sync_wait_for_fence(VkDevice device, VkSyncContext *ctx);

// Reset current frame fence
VkResult vk_sync_reset_fence(VkDevice device, VkSyncContext *ctx);

// Get current frame sync objects
FrameSync *vk_sync_get_current_frame(VkSyncContext *ctx);

// Advance to next frame
void vk_sync_advance_frame(VkSyncContext *ctx);

#endif // VK_SYNC_H
