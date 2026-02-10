#ifndef VK_COMMAND_H
#define VK_COMMAND_H

#include <vulkan/vulkan.h>
#include "utils/types.h"
#include "renderer/vk_sync.h"

// Command context
typedef struct VkCommandContext {
  VkCommandPool pool;
  VkCommandBuffer buffers[MAX_FRAMES_IN_FLIGHT];
} VkCommandContext;

// Create command pool and buffers
VkResult vk_command_create(
  VkDevice device, u32 queue_family_index, VkCommandContext *ctx);

// Destroy command context
void vk_command_destroy(VkDevice device, VkCommandContext *ctx);

// Get command buffer for current frame
VkCommandBuffer vk_command_get_buffer(VkCommandContext *ctx, u32 frame_index);

// Begin recording command buffer
VkResult vk_command_begin(VkCommandBuffer buffer);

// End recording command buffer
VkResult vk_command_end(VkCommandBuffer buffer);

// Begin single-time command buffer
VkCommandBuffer vk_command_begin_single(VkDevice device, VkCommandPool pool);

// End and submit single-time command buffer
VkResult vk_command_end_single(
  VkDevice device, VkCommandPool pool, VkQueue queue, VkCommandBuffer buffer);

#endif // VK_COMMAND_H
