#include "renderer_internal.h"

#include "core/log.h"
#include "geometry/mesh.h"

#include <string.h>

static void renderer_internal_destroy_camera_uniforms(Renderer *renderer) {
  for(u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    if(renderer->camera_uniform_mapped[i] && renderer->camera_uniform_buffers[i].memory != VK_NULL_HANDLE) {
      vkUnmapMemory(renderer->device.device, renderer->camera_uniform_buffers[i].memory);
      renderer->camera_uniform_mapped[i] = NULL;
    }
    vk_buffer_destroy(renderer->device.device, &renderer->camera_uniform_buffers[i]);
    renderer->camera_descriptor_sets[i] = VK_NULL_HANDLE;
  }

  if(renderer->descriptor_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(renderer->device.device, renderer->descriptor_pool, NULL);
    renderer->descriptor_pool = VK_NULL_HANDLE;
  }
}

static VkResult renderer_internal_create_camera_uniforms(Renderer *renderer) {
  memset(renderer->camera_uniform_buffers, 0, sizeof(renderer->camera_uniform_buffers));
  memset(renderer->camera_uniform_mapped, 0, sizeof(renderer->camera_uniform_mapped));
  memset(renderer->camera_descriptor_sets, 0, sizeof(renderer->camera_descriptor_sets));
  renderer->descriptor_pool = VK_NULL_HANDLE;

  for(u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkResult result = vk_buffer_create(&renderer->device,
                                       sizeof(CameraUniformData),
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &renderer->camera_uniform_buffers[i]);
    if(result != VK_SUCCESS) {
      LOG_ERROR("Failed to create camera uniform buffer %u: %d", i, result);
      renderer_internal_destroy_camera_uniforms(renderer);
      return result;
    }

    result = vkMapMemory(renderer->device.device,
                         renderer->camera_uniform_buffers[i].memory,
                         0,
                         sizeof(CameraUniformData),
                         0,
                         &renderer->camera_uniform_mapped[i]);
    if(result != VK_SUCCESS) {
      LOG_ERROR("Failed to map camera uniform buffer %u: %d", i, result);
      renderer_internal_destroy_camera_uniforms(renderer);
      return result;
    }
  }

  VkDescriptorPoolSize pool_size = {
    .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = MAX_FRAMES_IN_FLIGHT,
  };

  VkDescriptorPoolCreateInfo pool_info = {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .poolSizeCount = 1,
    .pPoolSizes    = &pool_size,
    .maxSets       = MAX_FRAMES_IN_FLIGHT,
  };

  VkResult result = vkCreateDescriptorPool(renderer->device.device, &pool_info, NULL, &renderer->descriptor_pool);
  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to create descriptor pool: %d", result);
    renderer_internal_destroy_camera_uniforms(renderer);
    return result;
  }

  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
  for(u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    layouts[i] = renderer->pipeline->global_set_layout;
  }

  VkDescriptorSetAllocateInfo alloc_info = {
    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool     = renderer->descriptor_pool,
    .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
    .pSetLayouts        = layouts,
  };

  result = vkAllocateDescriptorSets(renderer->device.device, &alloc_info, renderer->camera_descriptor_sets);
  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to allocate descriptor sets: %d", result);
    renderer_internal_destroy_camera_uniforms(renderer);
    return result;
  }

  for(u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkDescriptorBufferInfo buffer_info = {
      .buffer = renderer->camera_uniform_buffers[i].buffer,
      .offset = 0,
      .range  = sizeof(CameraUniformData),
    };

    VkWriteDescriptorSet descriptor_write = {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet          = renderer->camera_descriptor_sets[i],
      .dstBinding      = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pBufferInfo     = &buffer_info,
    };

    vkUpdateDescriptorSets(renderer->device.device, 1, &descriptor_write, 0, NULL);
  }

  return VK_SUCCESS;
}

Result renderer_upload_mesh(Renderer *renderer, const Mesh *mesh, MeshHandle *out_handle) {
  if(!renderer || !mesh || !out_handle) {
    return RESULT_ERROR_GENERIC;
  }

  if(renderer->mesh_count >= MAX_MESHES) {
    LOG_ERROR("Maximum mesh count (%d) reached", MAX_MESHES);
    return RESULT_ERROR_GENERIC;
  }

  u32      slot      = renderer->mesh_count;
  MeshGPU *mesh_gpu  = &renderer->meshes[slot];
  VkResult vk_result = VK_SUCCESS;

  vk_result = vk_buffer_create_vertex(&renderer->device,
                                      renderer->command.pool,
                                      mesh->vertices,
                                      mesh->vertex_count * sizeof(Vertex),
                                      &mesh_gpu->vertex_buffer);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create vertex buffer for mesh %u: %d", slot, vk_result);
    return RESULT_ERROR_VULKAN;
  }

  vk_result = vk_buffer_create_index(&renderer->device,
                                     renderer->command.pool,
                                     mesh->indices,
                                     mesh->index_count * sizeof(u32),
                                     &mesh_gpu->index_buffer);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create index buffer for mesh %u: %d", slot, vk_result);
    vk_buffer_destroy(renderer->device.device, &mesh_gpu->vertex_buffer);
    return RESULT_ERROR_VULKAN;
  }

  mesh_gpu->index_count = mesh->index_count;
  renderer->mesh_count++;
  *out_handle = slot;

  LOG_INFO("Uploaded mesh %u (%u vertices, %u indices)", slot, mesh->vertex_count, mesh->index_count);
  return RESULT_SUCCESS;
}

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

  vk_result = renderer_internal_create_camera_uniforms(renderer);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create camera uniform resources");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  vk_result = renderer_internal_create_framebuffers(renderer);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to create framebuffers");
    error = RESULT_ERROR_VULKAN;
    goto fail;
  }

  renderer->mesh_count = 0;

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

  if(has_device) {
    for(u32 i = 0; i < renderer->mesh_count; ++i) {
      vk_buffer_destroy(renderer->device.device, &renderer->meshes[i].index_buffer);
      vk_buffer_destroy(renderer->device.device, &renderer->meshes[i].vertex_buffer);
    }
    renderer->mesh_count = 0;
  }
  if(has_device) {
    renderer_internal_destroy_camera_uniforms(renderer);
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
