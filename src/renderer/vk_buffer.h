#ifndef VK_BUFFER_H
#define VK_BUFFER_H

#include <vulkan/vulkan.h>
#include "utils/types.h"
#include "renderer/vk_device.h"

// Buffer context
typedef struct VkBufferContext {
  VkBuffer       buffer;
  VkDeviceMemory memory;
  VkDeviceSize   size;
} VkBufferContext;

// Create a generic buffer
VkResult vk_buffer_create(VkDeviceContext *device, VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBufferContext *ctx);

// Destroy a buffer
void vk_buffer_destroy(VkDevice device, VkBufferContext *ctx);

// Copy data to buffer
VkResult vk_buffer_copy_data(VkDevice device, VkBufferContext *ctx, const void *data, VkDeviceSize size);

// Copy buffer to buffer
VkResult vk_buffer_copy(VkDeviceContext *device, VkCommandPool pool, VkBufferContext *src, VkBufferContext *dst,
                        VkDeviceSize size);

// Create vertex buffer (device local with staging)
VkResult vk_buffer_create_vertex(VkDeviceContext *device, VkCommandPool pool, const void *data, VkDeviceSize size,
                                 VkBufferContext *ctx);

// Create index buffer (device local with staging)
VkResult vk_buffer_create_index(VkDeviceContext *device, VkCommandPool pool, const void *data, VkDeviceSize size,
                                VkBufferContext *ctx);

#endif // VK_BUFFER_H
