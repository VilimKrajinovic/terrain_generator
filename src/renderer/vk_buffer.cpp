#include "vk_buffer.h"
#include "vk_command.h"
#include "core/log.h"
#include <string.h>

VkResult vk_buffer_create(
  VkDeviceContext *device, VkDeviceSize size, VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties, VkBufferContext *ctx)
{
  memset(ctx, 0, sizeof(VkBufferContext));
  ctx->size = size;

  // Create buffer
  VkBufferCreateInfo buffer_info = {
    .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size        = size,
    .usage       = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  VkResult result
    = vkCreateBuffer(device->device, &buffer_info, NULL, &ctx->buffer);
  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to create buffer: %d", result);
    return result;
  }

  // Get memory requirements
  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(
    device->device, ctx->buffer, &mem_requirements);

  // Allocate memory
  VkMemoryAllocateInfo alloc_info = {
    .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize  = mem_requirements.size,
    .memoryTypeIndex = vk_device_find_memory_type(
      device, mem_requirements.memoryTypeBits, properties),
  };

  if(alloc_info.memoryTypeIndex == UINT32_MAX) {
    vkDestroyBuffer(device->device, ctx->buffer, NULL);
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  }

  result = vkAllocateMemory(device->device, &alloc_info, NULL, &ctx->memory);
  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate buffer memory: %d", result);
    vkDestroyBuffer(device->device, ctx->buffer, NULL);
    return result;
  }

  // Bind memory to buffer
  result = vkBindBufferMemory(device->device, ctx->buffer, ctx->memory, 0);
  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to bind buffer memory: %d", result);
    vkFreeMemory(device->device, ctx->memory, NULL);
    vkDestroyBuffer(device->device, ctx->buffer, NULL);
    return result;
  }

  return VK_SUCCESS;
}

void vk_buffer_destroy(VkDevice device, VkBufferContext *ctx)
{
  if(ctx->buffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, ctx->buffer, NULL);
  }
  if(ctx->memory != VK_NULL_HANDLE) {
    vkFreeMemory(device, ctx->memory, NULL);
  }
  memset(ctx, 0, sizeof(VkBufferContext));
}

VkResult vk_buffer_copy_data(
  VkDevice device, VkBufferContext *ctx, const void *data, VkDeviceSize size)
{
  void *mapped;
  VkResult result = vkMapMemory(device, ctx->memory, 0, size, 0, &mapped);
  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to map buffer memory: %d", result);
    return result;
  }

  memcpy(mapped, data, (size_t)size);
  vkUnmapMemory(device, ctx->memory);

  return VK_SUCCESS;
}

VkResult vk_buffer_copy(
  VkDeviceContext *device, VkCommandPool pool, VkBufferContext *src,
  VkBufferContext *dst, VkDeviceSize size)
{
  VkCommandBuffer cmd = vk_command_begin_single(device->device, pool);
  if(cmd == VK_NULL_HANDLE) {
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }

  VkBufferCopy copy_region = {
    .srcOffset = 0,
    .dstOffset = 0,
    .size      = size,
  };

  vkCmdCopyBuffer(cmd, src->buffer, dst->buffer, 1, &copy_region);

  return vk_command_end_single(
    device->device, pool, device->graphics_queue, cmd);
}

VkResult vk_buffer_create_vertex(
  VkDeviceContext *device, VkCommandPool pool, const void *data,
  VkDeviceSize size, VkBufferContext *ctx)
{
  LOG_DEBUG("Creating vertex buffer (%llu bytes)", (unsigned long long)size);

  // Create staging buffer
  VkBufferContext staging;
  VkResult result = vk_buffer_create(
    device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &staging);
  if(result != VK_SUCCESS) {
    return result;
  }

  // Copy data to staging buffer
  result = vk_buffer_copy_data(device->device, &staging, data, size);
  if(result != VK_SUCCESS) {
    vk_buffer_destroy(device->device, &staging);
    return result;
  }

  // Create device local buffer
  result = vk_buffer_create(
    device, size,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ctx);
  if(result != VK_SUCCESS) {
    vk_buffer_destroy(device->device, &staging);
    return result;
  }

  // Copy from staging to device local
  result = vk_buffer_copy(device, pool, &staging, ctx, size);
  vk_buffer_destroy(device->device, &staging);

  if(result != VK_SUCCESS) {
    vk_buffer_destroy(device->device, ctx);
    return result;
  }

  LOG_DEBUG("Vertex buffer created");
  return VK_SUCCESS;
}

VkResult vk_buffer_create_index(
  VkDeviceContext *device, VkCommandPool pool, const void *data,
  VkDeviceSize size, VkBufferContext *ctx)
{
  LOG_DEBUG("Creating index buffer (%llu bytes)", (unsigned long long)size);

  // Create staging buffer
  VkBufferContext staging;
  VkResult result = vk_buffer_create(
    device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    &staging);
  if(result != VK_SUCCESS) {
    return result;
  }

  // Copy data to staging buffer
  result = vk_buffer_copy_data(device->device, &staging, data, size);
  if(result != VK_SUCCESS) {
    vk_buffer_destroy(device->device, &staging);
    return result;
  }

  // Create device local buffer
  result = vk_buffer_create(
    device, size,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ctx);
  if(result != VK_SUCCESS) {
    vk_buffer_destroy(device->device, &staging);
    return result;
  }

  // Copy from staging to device local
  result = vk_buffer_copy(device, pool, &staging, ctx, size);
  vk_buffer_destroy(device->device, &staging);

  if(result != VK_SUCCESS) {
    vk_buffer_destroy(device->device, ctx);
    return result;
  }

  LOG_DEBUG("Index buffer created");
  return VK_SUCCESS;
}
