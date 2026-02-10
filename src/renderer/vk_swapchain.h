#ifndef VK_SWAPCHAIN_H
#define VK_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include "utils/types.h"
#include "memory/arena.h"
#include "renderer/vk_device.h"

// Maximum number of swapchain images
#define MAX_SWAPCHAIN_IMAGES 4

// Swapchain support details
typedef struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR      *formats;
  u32                      format_count;
  VkPresentModeKHR        *present_modes;
  u32                      present_mode_count;
} SwapchainSupportDetails;

// Swapchain context
typedef struct VkSwapchainContext {
  VkSwapchainKHR swapchain;
  VkImage        images[MAX_SWAPCHAIN_IMAGES];
  VkImageView    image_views[MAX_SWAPCHAIN_IMAGES];
  u32            image_count;
  VkFormat       format;
  VkExtent2D     extent;
} VkSwapchainContext;

// Query swapchain support (allocates from arena)
SwapchainSupportDetails vk_swapchain_query_support(
  VkPhysicalDevice device, VkSurfaceKHR surface, Arena *arena);

// Create swapchain
VkResult vk_swapchain_create(
  VkDeviceContext *device, VkSurfaceKHR surface, u32 width, u32 height,
  VkSwapchainKHR old_swapchain, VkSwapchainContext *ctx);

// Destroy swapchain
void vk_swapchain_destroy(VkDeviceContext *device, VkSwapchainContext *ctx);

// Recreate swapchain (for resize)
VkResult vk_swapchain_recreate(
  VkDeviceContext *device, VkSurfaceKHR surface, u32 width, u32 height,
  VkSwapchainContext *ctx);

#endif // VK_SWAPCHAIN_H
