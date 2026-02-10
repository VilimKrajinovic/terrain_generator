#include "renderer_internal.h"

#include "core/log.h"
#include "geometry/quad.h"

#include <string.h>

Result renderer_create(Renderer **out_renderer, WindowContext *window, const RendererConfig *config, Arena *arena) {
  if(!out_renderer || !window || !config || !arena) {
    return RESULT_ERROR_GENERIC;
  }

  *out_renderer = NULL;

  Renderer *renderer = ARENA_PUSH_STRUCT(arena, Renderer);
  if(!renderer) {
    LOG_ERROR("Failed to allocate renderer");
    return RESULT_ERROR_OUT_OF_MEMORY;
  }

  memset(renderer, 0, sizeof(*renderer));
  renderer->window = window;

  LOG_INFO("Initializing renderer");

  Result   error     = RESULT_SUCCESS;
  VkResult vk_result = VK_SUCCESS;
  u32      width     = 0;
  u32      height    = 0;

  VkInstanceConfig instance_config = {
    .app_name          = config->app_name,
    .app_version       = VK_MAKE_VERSION(1, 0, 0),
    .enable_validation = config->enable_validation,
  };

  vk_result = vk_instance_create(&instance_config, &renderer->instance);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create Vulkan instance");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  vk_result = window_create_surface(window, renderer->instance.instance, &renderer->surface);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create window surface");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  vk_result = vk_device_create(renderer->instance.instance, renderer->surface, &renderer->device);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create device");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  window_get_framebuffer_size(window, &width, &height);

  vk_result
    = vk_swapchain_create(&renderer->device, renderer->surface, width, height, VK_NULL_HANDLE, &renderer->swapchain);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create swapchain");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  memset(renderer->images_in_flight, 0, sizeof(renderer->images_in_flight));
  memset(renderer->render_finished, 0, sizeof(renderer->render_finished));

  vk_result = renderer_internal_create_render_finished(renderer);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create render-finished semaphores");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  vk_result = vk_sync_create(renderer->device.device, &renderer->sync);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create sync objects");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  vk_result
    = vk_command_create(renderer->device.device, renderer->device.queue_families.graphics_family, &renderer->command);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create command pool");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  renderer->render_pass = ARENA_PUSH_STRUCT(arena, VkRenderPassContext);
  if(!renderer->render_pass) {
    LOG_ERROR("Failed to allocate render pass context");
    error = RESULT_ERROR_OUT_OF_MEMORY;
    goto fail;
  }

  vk_result = vk_renderpass_create(renderer->device.device, renderer->swapchain.format, renderer->render_pass);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create render pass");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  renderer->pipeline = ARENA_PUSH_STRUCT(arena, VkPipelineContext);
  if(!renderer->pipeline) {
    LOG_ERROR("Failed to allocate pipeline context");
    error = RESULT_ERROR_OUT_OF_MEMORY;
    goto fail;
  }

  vk_result = vk_pipeline_create(
    renderer->device.device, renderer->render_pass->render_pass, renderer->swapchain.extent, renderer->pipeline);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create graphics pipeline");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  vk_result = renderer_internal_create_framebuffers(renderer);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create framebuffers");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  renderer->quad_mesh = ARENA_PUSH_STRUCT(arena, Mesh);
  if(!renderer->quad_mesh) {
    LOG_ERROR("Failed to allocate mesh");
    error = RESULT_ERROR_OUT_OF_MEMORY;
    goto fail;
  }

  quad_create_arena(renderer->quad_mesh, arena);
  if(!renderer->quad_mesh->vertices || !renderer->quad_mesh->indices) {
    LOG_ERROR("Failed to create quad mesh");
    error = RESULT_ERROR_OUT_OF_MEMORY;
    goto fail;
  }

  renderer->vertex_buffer = ARENA_PUSH_STRUCT(arena, VkBufferContext);
  if(!renderer->vertex_buffer) {
    LOG_ERROR("Failed to allocate vertex buffer context");
    error = RESULT_ERROR_OUT_OF_MEMORY;
    goto fail;
  }

  vk_result = vk_buffer_create_vertex(&renderer->device,
                                      renderer->command.pool,
                                      renderer->quad_mesh->vertices,
                                      renderer->quad_mesh->vertex_count * sizeof(Vertex),
                                      renderer->vertex_buffer);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create vertex buffer");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  renderer->index_buffer = ARENA_PUSH_STRUCT(arena, VkBufferContext);
  if(!renderer->index_buffer) {
    LOG_ERROR("Failed to allocate index buffer context");
    error = RESULT_ERROR_OUT_OF_MEMORY;
    goto fail;
  }

  vk_result = vk_buffer_create_index(&renderer->device,
                                     renderer->command.pool,
                                     renderer->quad_mesh->indices,
                                     renderer->quad_mesh->index_count * sizeof(u32),
                                     renderer->index_buffer);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create index buffer");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  *out_renderer = renderer;
  LOG_INFO("Renderer initialized successfully");
  return RESULT_SUCCESS;

fail:
  renderer_destroy(renderer);
  return error;
}

void renderer_destroy(Renderer *renderer) {
  if(!renderer) {
    return;
  }

  LOG_INFO("Shutting down renderer");

  const bool has_device = renderer->device.device != VK_NULL_HANDLE;

  if(has_device) {
    vk_device_wait_idle(&renderer->device);
  }

  if(has_device && renderer->index_buffer) {
    vk_buffer_destroy(renderer->device.device, renderer->index_buffer);
  }
  if(has_device && renderer->vertex_buffer) {
    vk_buffer_destroy(renderer->device.device, renderer->vertex_buffer);
  }

  if(has_device) {
    renderer_internal_destroy_framebuffers(renderer);
  }

  if(has_device && renderer->pipeline) {
    vk_pipeline_destroy(renderer->device.device, renderer->pipeline);
  }

  if(has_device && renderer->render_pass) {
    vk_renderpass_destroy(renderer->device.device, renderer->render_pass);
  }

  if(has_device) {
    vk_command_destroy(renderer->device.device, &renderer->command);
    vk_sync_destroy(renderer->device.device, &renderer->sync);
    renderer_internal_destroy_render_finished(renderer);
    vk_swapchain_destroy(&renderer->device, &renderer->swapchain);
    vk_device_destroy(&renderer->device);
  }

  if(renderer->surface != VK_NULL_HANDLE && renderer->instance.instance != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(renderer->instance.instance, renderer->surface, NULL);
  }

  if(renderer->instance.instance != VK_NULL_HANDLE) {
    vk_instance_destroy(&renderer->instance);
  }

  memset(renderer, 0, sizeof(*renderer));
  LOG_INFO("Renderer shutdown complete");
}
