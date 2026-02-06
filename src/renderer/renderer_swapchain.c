#include "renderer_internal.h"

#include "core/log.h"

#include <string.h>

VkResult renderer_internal_create_framebuffers(Renderer *renderer) {
  renderer->framebuffer_count = renderer->swapchain.image_count;
  if (renderer->framebuffer_count > MAX_SWAPCHAIN_IMAGES) {
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  for (u32 i = 0; i < renderer->framebuffer_count; i++) {
    VkImageView attachments[] = {renderer->swapchain.image_views[i]};

    VkFramebufferCreateInfo fb_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderer->render_pass->render_pass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = renderer->swapchain.extent.width,
        .height = renderer->swapchain.extent.height,
        .layers = 1,
    };

    VkResult vk_result = vkCreateFramebuffer(renderer->device.device, &fb_info,
                                             NULL, &renderer->framebuffers[i]);
    if (vk_result != VK_SUCCESS) {
      LOG_ERROR("Failed to create framebuffer %u: %d", i, vk_result);
      return vk_result;
    }
  }

  LOG_DEBUG("Created %u framebuffers", renderer->framebuffer_count);
  return VK_SUCCESS;
}

void renderer_internal_destroy_framebuffers(Renderer *renderer) {
  for (u32 i = 0; i < renderer->framebuffer_count; i++) {
    if (renderer->framebuffers[i] != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(renderer->device.device, renderer->framebuffers[i],
                           NULL);
    }
  }

  renderer->framebuffer_count = 0;
}

VkResult renderer_internal_create_render_finished(Renderer *renderer) {
  VkSemaphoreCreateInfo semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  for (u32 i = 0; i < renderer->swapchain.image_count; i++) {
    VkResult vk_result =
        vkCreateSemaphore(renderer->device.device, &semaphore_info, NULL,
                          &renderer->render_finished[i]);
    if (vk_result != VK_SUCCESS) {
      for (u32 j = 0; j < i; j++) {
        vkDestroySemaphore(renderer->device.device, renderer->render_finished[j],
                           NULL);
        renderer->render_finished[j] = VK_NULL_HANDLE;
      }
      return vk_result;
    }
  }

  return VK_SUCCESS;
}

void renderer_internal_destroy_render_finished(Renderer *renderer) {
  for (u32 i = 0; i < renderer->swapchain.image_count; i++) {
    if (renderer->render_finished[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(renderer->device.device, renderer->render_finished[i],
                         NULL);
      renderer->render_finished[i] = VK_NULL_HANDLE;
    }
  }
}

VkResult renderer_internal_recreate_swapchain(Renderer *renderer, u32 width,
                                              u32 height) {
  LOG_INFO("Recreating swapchain (%ux%u)", width, height);

  renderer_internal_destroy_framebuffers(renderer);
  renderer_internal_destroy_render_finished(renderer);

  VkResult vk_result = vk_swapchain_recreate(
      &renderer->device, renderer->surface, width, height, &renderer->swapchain);
  if (vk_result != VK_SUCCESS) {
    return vk_result;
  }

  memset(renderer->images_in_flight, 0, sizeof(renderer->images_in_flight));
  memset(renderer->render_finished, 0, sizeof(renderer->render_finished));

  vk_result = renderer_internal_create_render_finished(renderer);
  if (vk_result != VK_SUCCESS) {
    return vk_result;
  }

  return renderer_internal_create_framebuffers(renderer);
}
