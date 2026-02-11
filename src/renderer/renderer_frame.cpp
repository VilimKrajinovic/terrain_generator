#include "foundation/result.h"
#include "renderer_internal.h"

#include "camera/camera.h"
#include "core/log.h"
#include "utils/macros.h"
#include <string.h>

const i32 ONE_SECOND = 1000000000;

Result renderer_draw(Renderer *renderer, const Camera *camera) {
  if(!renderer || !camera) {
    return RESULT_ERROR_GENERIC;
  }

  if(renderer->swapchain_needs_recreation) {
    return renderer_resize(renderer);
  }

  VkResult vk_result = vk_sync_wait_for_fence(renderer->device.device, &renderer->sync);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed waiting for in-flight fence: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  FrameSync *frame_sync  = vk_sync_get_current_frame(&renderer->sync);
  u32        image_index = 0;
  u32        frame_index = renderer->sync.current_frame;

  vk_result = vkAcquireNextImageKHR(renderer->device.device,
                                    renderer->swapchain.swapchain,
                                    ONE_SECOND,
                                    frame_sync->image_available,
                                    VK_NULL_HANDLE,
                                    &image_index);
  if(vk_result == VK_ERROR_OUT_OF_DATE_KHR) {
    renderer->swapchain_needs_recreation = true;
    return renderer_resize(renderer);
  }
  if(vk_result != VK_SUCCESS && vk_result != VK_SUBOPTIMAL_KHR) {
    LOG_ERROR("Failed to acquire swapchain image: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  if(renderer->images_in_flight[image_index] != VK_NULL_HANDLE) {
    vk_result
      = vkWaitForFences(renderer->device.device, 1, &renderer->images_in_flight[image_index], VK_TRUE, ONE_SECOND);
    if(vk_result != VK_SUCCESS) {
      LOG_ERROR("Failed waiting for image fence: %d", vk_result);
      return RESULT_ERROR_VULKAN;
    }
  }

  vk_result = vk_sync_reset_fence(renderer->device.device, &renderer->sync);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to reset in-flight fence: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }
  renderer->images_in_flight[image_index] = frame_sync->in_flight;

  const f32 aspect = renderer->swapchain.extent.height > 0
                     ? (f32)renderer->swapchain.extent.width / (f32)renderer->swapchain.extent.height
                     : 1.0f;

  CameraUniformData uniform_data = {};
  uniform_data.view              = view_matrix(*camera);
  uniform_data.proj              = glm::perspective(glm::radians(camera->zoom), aspect, 0.1f, 100.0f);
  uniform_data.proj[1][1] *= -1.0f;
  uniform_data.camera_pos = glm::vec4(camera->position, 1.0f);

  memcpy(renderer->camera_uniform_mapped[frame_index], &uniform_data, sizeof(uniform_data));

  VkCommandBuffer cmd = vk_command_get_buffer(&renderer->command, frame_index);

  VK_CHECK_RETURN(vk_command_begin(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT), RESULT_ERROR_VULKAN);

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
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->pipeline->pipeline);
  vkCmdBindDescriptorSets(cmd,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          renderer->pipeline->layout,
                          0,
                          1,
                          &renderer->camera_descriptor_sets[frame_index],
                          0,
                          NULL);

  VkViewport viewport = {
    .x        = 0.0f,
    .y        = 0.0f,
    .width    = (float)renderer->swapchain.extent.width,
    .height   = (float)renderer->swapchain.extent.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor = {
    .offset = {0, 0},
    .extent = renderer->swapchain.extent,
  };
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  VkBuffer     vertex_buffers[] = {renderer->vertex_buffer->buffer};
  VkDeviceSize offsets[]        = {0};
  vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(cmd, renderer->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(cmd, renderer->quad_mesh->index_count, 1, 0, 0, 0);

  vkCmdEndRenderPass(cmd);

  vk_result = vk_command_end(cmd);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to end command buffer: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  VkSemaphore          wait_semaphores[]   = {frame_sync->image_available};
  VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore          signal_semaphores[] = {renderer->render_finished[image_index]};

  VkSubmitInfo submit_info = {
    .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount   = 1,
    .pWaitSemaphores      = wait_semaphores,
    .pWaitDstStageMask    = wait_stages,
    .commandBufferCount   = 1,
    .pCommandBuffers      = &cmd,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores    = signal_semaphores,
  };

  vk_result = vkQueueSubmit(renderer->device.graphics_queue, 1, &submit_info, frame_sync->in_flight);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to submit draw command buffer: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  VkSwapchainKHR   swapchains[] = {renderer->swapchain.swapchain};
  VkPresentInfoKHR present_info = {
    .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores    = signal_semaphores,
    .swapchainCount     = 1,
    .pSwapchains        = swapchains,
    .pImageIndices      = &image_index,
  };

  vk_result = vkQueuePresentKHR(renderer->device.present_queue, &present_info);
  if(vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
    renderer->swapchain_needs_recreation = true;
    return renderer_resize(renderer);
  }
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to present swapchain image: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  vk_sync_advance_frame(&renderer->sync);
  return RESULT_SUCCESS;
}

Result renderer_resize(Renderer *renderer) {
  if(!renderer || !renderer->window) {
    return RESULT_ERROR_GENERIC;
  }

  u32 width  = 0;
  u32 height = 0;
  window_get_framebuffer_size(renderer->window, &width, &height);

  if(width == 0 || height == 0) {
    renderer->swapchain_needs_recreation = true;
    return RESULT_SUCCESS;
  }

  vk_device_wait_idle(&renderer->device);

  VkResult vk_result = renderer_internal_recreate_swapchain(renderer, width, height);
  if(vk_result != VK_SUCCESS) {
    LOG_ERROR("Failed to recreate swapchain: %d", vk_result);
    return RESULT_ERROR_VULKAN;
  }

  renderer->swapchain_needs_recreation = false;
  return RESULT_SUCCESS;
}

void renderer_wait_idle(Renderer *renderer) {
  if(!renderer) {
    return;
  }
  vk_device_wait_idle(&renderer->device);
}
