#ifndef VK_DEVICE_H
#define VK_DEVICE_H

#include <vulkan/vulkan.h>
#include "utils/types.h"

// Queue family indices
typedef struct QueueFamilyIndices {
  u32  graphics_family;
  u32  present_family;
  bool graphics_family_found;
  bool present_family_found;
} QueueFamilyIndices;

// Device context
typedef struct VkDeviceContext {
  VkPhysicalDevice                 physical_device;
  VkDevice                         device;
  VkQueue                          graphics_queue;
  VkQueue                          present_queue;
  QueueFamilyIndices               queue_families;
  VkPhysicalDeviceProperties       properties;
  VkPhysicalDeviceFeatures         features;
  VkPhysicalDeviceMemoryProperties memory_properties;
} VkDeviceContext;

// Create device (selects physical device and creates logical device)
VkResult vk_device_create(
  VkInstance instance, VkSurfaceKHR surface, VkDeviceContext *ctx);

// Destroy device
void vk_device_destroy(VkDeviceContext *ctx);

// Find queue families
QueueFamilyIndices
vk_device_find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);

// Check if queue families are complete
bool vk_device_queue_families_complete(const QueueFamilyIndices *indices);

// Find memory type
u32 vk_device_find_memory_type(
  VkDeviceContext *ctx, u32 type_filter, VkMemoryPropertyFlags properties);

// Wait for device idle
void vk_device_wait_idle(VkDeviceContext *ctx);

#endif // VK_DEVICE_H
