#ifndef COMPUTE_H
#define COMPUTE_H

#include "utils/types.h"
#include <vulkan/vulkan.h>

// Placeholder for future compute shader support
// This will be used for terrain generation and corrosion simulation

typedef struct ComputeContext {
  VkPipeline            pipeline;
  VkPipelineLayout      layout;
  VkDescriptorSetLayout descriptor_layout;
  VkDescriptorPool      descriptor_pool;
  VkDescriptorSet       descriptor_set;
} ComputeContext;

#endif // COMPUTE_H
