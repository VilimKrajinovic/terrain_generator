#ifndef VK_SHADER_H
#define VK_SHADER_H

#include <vulkan/vulkan.h>
#include "utils/types.h"

// Load shader module from SPIR-V file
VkResult vk_shader_load(VkDevice device, const char *path, VkShaderModule *module);

// Destroy shader module
void vk_shader_destroy(VkDevice device, VkShaderModule module);

// Create shader stage info
VkPipelineShaderStageCreateInfo vk_shader_stage_info(VkShaderStageFlagBits stage, VkShaderModule module);

#endif // VK_SHADER_H
