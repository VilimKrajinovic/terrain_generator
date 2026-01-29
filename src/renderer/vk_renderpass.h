#ifndef VK_RENDERPASS_H
#define VK_RENDERPASS_H

#include <vulkan/vulkan.h>
#include "utils/types.h"

// Render pass context
typedef struct VkRenderPassContext {
    VkRenderPass render_pass;
} VkRenderPassContext;

// Create render pass
VkResult vk_renderpass_create(VkDevice device, VkFormat color_format, VkRenderPassContext *ctx);

// Destroy render pass
void vk_renderpass_destroy(VkDevice device, VkRenderPassContext *ctx);

#endif // VK_RENDERPASS_H
