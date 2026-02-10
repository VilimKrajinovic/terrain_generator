#include "vk_shader.h"
#include "core/log.h"
#include "utils/file_io.h"
#include "memory/arena.h"

VkResult
vk_shader_load(VkDevice device, const char *path, VkShaderModule *module)
{
  LOG_DEBUG("Loading shader: %s", path);

  // Read SPIR-V file using scratch arena
  ArenaTemp scratch = arena_scratch_begin();
  size_t    code_size;
  u8       *code = file_read_binary_arena(path, &code_size, scratch.arena);
  if(!code) {
    LOG_ERROR("Failed to read shader file: %s", path);
    arena_temp_end(scratch);
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  // Create shader module
  VkShaderModuleCreateInfo create_info = {
    .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = code_size,
    .pCode    = (const u32 *)code,
  };

  VkResult result = vkCreateShaderModule(device, &create_info, NULL, module);
  arena_temp_end(scratch); // Free shader code

  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to create shader module: %d", result);
    return result;
  }

  LOG_DEBUG("Shader loaded successfully: %s", path);
  return VK_SUCCESS;
}

void vk_shader_destroy(VkDevice device, VkShaderModule module)
{
  if(module != VK_NULL_HANDLE) {
    vkDestroyShaderModule(device, module, NULL);
  }
}

VkPipelineShaderStageCreateInfo
vk_shader_stage_info(VkShaderStageFlagBits stage, VkShaderModule module)
{
  return VkPipelineShaderStageCreateInfo{
    .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage               = stage,
    .module              = module,
    .pName               = "main",
    .pSpecializationInfo = NULL,
  };
}
