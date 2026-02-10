#ifndef VK_INSTANCE_H
#define VK_INSTANCE_H

#include <vulkan/vulkan.h>
#include "utils/types.h"

// Instance configuration
typedef struct VkInstanceConfig {
  const char *app_name;
  u32 app_version;
  bool enable_validation;
} VkInstanceConfig;

// Instance context
typedef struct VkInstanceContext {
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  bool validation_enabled;
} VkInstanceContext;

// Create Vulkan instance
VkResult
vk_instance_create(const VkInstanceConfig *config, VkInstanceContext *ctx);

// Destroy Vulkan instance
void vk_instance_destroy(VkInstanceContext *ctx);

// Check if validation layers are available
bool vk_instance_check_validation_support(void);

#endif // VK_INSTANCE_H
