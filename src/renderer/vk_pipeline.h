#ifndef VK_PIPELINE_H
#define VK_PIPELINE_H

#include <vulkan/vulkan.h>
#include "utils/types.h"

// Pipeline context
typedef struct VkPipelineContext {
  VkPipelineLayout layout;
  VkPipeline       pipeline;
} VkPipelineContext;

// Create graphics pipeline
VkResult vk_pipeline_create(
  VkDevice device, VkRenderPass render_pass, VkExtent2D extent,
  VkPipelineContext *ctx);

// Destroy pipeline
void vk_pipeline_destroy(VkDevice device, VkPipelineContext *ctx);

#endif // VK_PIPELINE_H
