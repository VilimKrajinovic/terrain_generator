#include "renderer_internal.h"

#include "core/log.h"

Result renderer_draw(Renderer *renderer) {
  if (!renderer) {
    return RESULT_ERROR_GENERIC;
  }

  if (renderer->swapchain_needs_recreation) {
    return renderer_resize(renderer);
  }

  VkResult vk_result =
      vk_sync_wait_for_fence(renderer->device.device, &renderer->sync);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed waiting for in-flight fence: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  FrameSync *frame = vk_sync_get_current_frame(&renderer->sync);
  u32 image_index = 0;

  vk_result =
      vkAcquireNextImageKHR(renderer->device.device, renderer->swapchain.swapchain,
                            UINT64_MAX, frame->image_available, VK_NULL_HANDLE,
                            &image_index);
  if (vk_result == VK_ERROR_OUT_OF_DATE_KHR) {
    renderer->swapchain_needs_recreation = true;
    return renderer_resize(renderer);
  }
  if (vk_result != VK_SUCCESS && vk_result != VK_SUBOPTIMAL_KHR) {
    LOG_ERROR("Failed to acquire swapchain image: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  if (renderer->images_in_flight[image_index] != VK_NULL_HANDLE) {
    vk_result = vkWaitForFences(renderer->device.device, 1,
                                &renderer->images_in_flight[image_index],
                                VK_TRUE, UINT64_MAX);
    if (vk_result != VK_SUCCESS) {
      LOG_ERROR("Failed waiting for image fence: %d", vk_result);
      return RESULT_ERROR_VULKAN;
    }
  }

  vk_result = vk_sync_reset_fence(renderer->device.device, &renderer->sync);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to reset in-flight fence: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }
  renderer->images_in_flight[image_index] = frame->in_flight;

  VkCommandBuffer cmd =
      vk_command_get_buffer(&renderer->command, renderer->sync.current_frame);
  vk_result = vk_command_begin(cmd);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to begin command buffer: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  VkClearValue clear_color = {{{0.1f, 0.1f, 0.15f, 1.0f}}};

  VkRenderPassBeginInfo render_pass_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = renderer->render_pass->render_pass,
      .framebuffer = renderer->framebuffers[image_index],
      .renderArea =
          {
              .offset = {0, 0},
              .extent = renderer->swapchain.extent,
          },
      .clearValueCount = 1,
      .pClearValues = &clear_color,
  };

  vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderer->pipeline->pipeline);

  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)renderer->swapchain.extent.width,
      .height = (float)renderer->swapchain.extent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = renderer->swapchain.extent,
  };
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  VkBuffer vertex_buffers[] = {renderer->vertex_buffer->buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(cmd, renderer->index_buffer->buffer, 0,
                       VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(cmd, renderer->quad_mesh->index_count, 1, 0, 0, 0);

  vkCmdEndRenderPass(cmd);

  vk_result = vk_command_end(cmd);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to end command buffer: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  VkSemaphore wait_semaphores[] = {frame->image_available};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signal_semaphores[] = {renderer->render_finished[image_index]};

  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = wait_semaphores,
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = signal_semaphores,
  };

  vk_result = vkQueueSubmit(renderer->device.graphics_queue, 1, &submit_info,
                            frame->in_flight);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to submit draw command buffer: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  VkSwapchainKHR swapchains[] = {renderer->swapchain.swapchain};
  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = signal_semaphores,
      .swapchainCount = 1,
      .pSwapchains = swapchains,
      .pImageIndices = &image_index,
  };

  vk_result = vkQueuePresentKHR(renderer->device.present_queue, &present_info);
  if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
    renderer->swapchain_needs_recreation = true;
    return renderer_resize(renderer);
  }
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to present swapchain image: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  vk_sync_advance_frame(&renderer->sync);
  return RESULT_SUCCESS;
}

Result renderer_resize(Renderer *renderer) {
  if (!renderer || !renderer->window) {
    return RESULT_ERROR_GENERIC;
  }

  u32 width = 0;
  u32 height = 0;
  window_get_framebuffer_size(renderer->window, &width, &height);

  if (width == 0 || height == 0) {
    renderer->swapchain_needs_recreation = true;
    return RESULT_SUCCESS;
  }

  vk_device_wait_idle(&renderer->device);

  VkResult vk_result =
      renderer_internal_recreate_swapchain(renderer, width, height);
  if (vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to recreate swapchain: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  renderer->swapchain_needs_recreation = false;
  return RESULT_SUCCESS;
}

void renderer_wait_idle(Renderer *renderer) {
  if (!renderer) {
    return;
  }
  vk_device_wait_idle(&renderer->device);
}
