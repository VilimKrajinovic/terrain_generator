#ifndef RENDERER_INTERNAL_H
#define RENDERER_INTERNAL_H

#include "renderer/renderer.h"
#include "renderer/vk_buffer.h"
#include "renderer/vk_command.h"
#include "renderer/vk_device.h"
#include "renderer/vk_instance.h"
#include "renderer/vk_pipeline.h"
#include "renderer/vk_renderpass.h"
#include "renderer/vk_swapchain.h"
#include "renderer/vk_sync.h"
#include "glm/glm.hpp"

#define MAX_MESHES 64

typedef u32 MeshHandle;

typedef struct MeshGPU {
  VkBufferContext vertex_buffer;
  VkBufferContext index_buffer;
  u32             index_count;
} MeshGPU;

typedef struct CameraUniformData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::vec4 camera_pos;
} CameraUniformData;

struct Renderer {
  VkInstanceContext  instance;
  VkSurfaceKHR       surface;
  VkDeviceContext    device;
  VkSwapchainContext swapchain;
  VkSyncContext      sync;
  VkCommandContext   command;

  VkRenderPassContext *render_pass;
  VkPipelineContext   *pipeline;

  VkFramebuffer    framebuffers[MAX_SWAPCHAIN_IMAGES];
  u32              framebuffer_count;
  VkFence          images_in_flight[MAX_SWAPCHAIN_IMAGES];
  VkSemaphore      render_finished[MAX_SWAPCHAIN_IMAGES];
  VkBufferContext  camera_uniform_buffers[MAX_FRAMES_IN_FLIGHT];
  void            *camera_uniform_mapped[MAX_FRAMES_IN_FLIGHT];
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet  camera_descriptor_sets[MAX_FRAMES_IN_FLIGHT];

  MeshGPU meshes[MAX_MESHES];
  u32     mesh_count;

  WindowContext *window;
  bool           swapchain_needs_recreation;
};

VkResult renderer_internal_create_framebuffers(Renderer *renderer);
void     renderer_internal_destroy_framebuffers(Renderer *renderer);
VkResult renderer_internal_recreate_swapchain(Renderer *renderer, u32 width, u32 height);

VkResult renderer_internal_create_render_finished(Renderer *renderer);
void     renderer_internal_destroy_render_finished(Renderer *renderer);

#endif // RENDERER_INTERNAL_H
