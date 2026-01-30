#include "renderer.h"
#include "core/log.h"
#include "utils/macros.h"
#include "renderer/vk_renderpass.h"
#include "renderer/vk_pipeline.h"
#include "renderer/vk_buffer.h"
#include "geometry/mesh.h"
#include "geometry/quad.h"
#include <string.h>

// Forward declarations for internal functions
static VkResult create_framebuffers(RendererContext *ctx, Arena *arena);
static void destroy_framebuffers(RendererContext *ctx);
static VkResult recreate_swapchain(RendererContext *ctx, u32 width, u32 height);
static VkResult create_render_finished(RendererContext *ctx);
static void destroy_render_finished(RendererContext *ctx);

Result renderer_init(RendererContext *ctx, WindowContext *window, const RendererConfig *config, Arena *arena) {
    LOG_INFO("Initializing renderer");

    memset(ctx, 0, sizeof(RendererContext));
    ctx->arena = arena;

    // Create Vulkan instance
    VkInstanceConfig instance_config = {
        .app_name = config->app_name,
        .app_version = VK_MAKE_VERSION(1, 0, 0),
        .enable_validation = config->enable_validation,
    };

    VkResult result = vk_instance_create(&instance_config, &ctx->instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance");
        return RESULT_ERROR_VULKAN;
    }

    // Create surface
    result = window_create_surface(window, ctx->instance.instance, &ctx->surface);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create window surface");
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    // Create device
    result = vk_device_create(ctx->instance.instance, ctx->surface, &ctx->device);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create device");
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    // Create swapchain
    u32 width, height;
    window_get_framebuffer_size(window, &width, &height);

    result = vk_swapchain_create(&ctx->device, ctx->surface, width, height, VK_NULL_HANDLE, &ctx->swapchain);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create swapchain");
        vk_device_destroy(&ctx->device);
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    memset(ctx->images_in_flight, 0, sizeof(ctx->images_in_flight));
    memset(ctx->render_finished, 0, sizeof(ctx->render_finished));

    result = create_render_finished(ctx);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create render finished semaphores");
        vk_swapchain_destroy(&ctx->device, &ctx->swapchain);
        vk_device_destroy(&ctx->device);
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    // Create sync objects
    result = vk_sync_create(ctx->device.device, &ctx->sync);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create sync objects");
        destroy_render_finished(ctx);
        vk_swapchain_destroy(&ctx->device, &ctx->swapchain);
        vk_device_destroy(&ctx->device);
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    // Create command pool and buffers
    result = vk_command_create(ctx->device.device, ctx->device.queue_families.graphics_family, &ctx->command);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create command pool");
        vk_sync_destroy(ctx->device.device, &ctx->sync);
        vk_swapchain_destroy(&ctx->device, &ctx->swapchain);
        vk_device_destroy(&ctx->device);
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    // Create render pass
    ctx->render_pass = ARENA_PUSH_STRUCT(arena, VkRenderPassContext);
    result = vk_renderpass_create(ctx->device.device, ctx->swapchain.format, ctx->render_pass);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create render pass");
        vk_command_destroy(ctx->device.device, &ctx->command);
        vk_sync_destroy(ctx->device.device, &ctx->sync);
        vk_swapchain_destroy(&ctx->device, &ctx->swapchain);
        vk_device_destroy(&ctx->device);
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    // Create pipeline
    ctx->pipeline = ARENA_PUSH_STRUCT(arena, VkPipelineContext);
    result = vk_pipeline_create(ctx->device.device, ctx->render_pass->render_pass,
                                ctx->swapchain.extent, ctx->pipeline);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create graphics pipeline");
        vk_renderpass_destroy(ctx->device.device, ctx->render_pass);
        vk_command_destroy(ctx->device.device, &ctx->command);
        vk_sync_destroy(ctx->device.device, &ctx->sync);
        vk_swapchain_destroy(&ctx->device, &ctx->swapchain);
        vk_device_destroy(&ctx->device);
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    // Create framebuffers
    result = create_framebuffers(ctx, arena);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create framebuffers");
        vk_pipeline_destroy(ctx->device.device, ctx->pipeline);
        vk_renderpass_destroy(ctx->device.device, ctx->render_pass);
        vk_command_destroy(ctx->device.device, &ctx->command);
        vk_sync_destroy(ctx->device.device, &ctx->sync);
        vk_swapchain_destroy(&ctx->device, &ctx->swapchain);
        vk_device_destroy(&ctx->device);
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
        vk_instance_destroy(&ctx->instance);
        return RESULT_ERROR_VULKAN;
    }

    // Create quad mesh
    ctx->quad_mesh = ARENA_PUSH_STRUCT(arena, Mesh);
    quad_create_arena(ctx->quad_mesh, arena);

    // Create vertex buffer
    ctx->vertex_buffer = ARENA_PUSH_STRUCT(arena, VkBufferContext);
    result = vk_buffer_create_vertex(&ctx->device, ctx->command.pool,
                                     ctx->quad_mesh->vertices,
                                     ctx->quad_mesh->vertex_count * sizeof(Vertex),
                                     ctx->vertex_buffer);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create vertex buffer");
        // Cleanup...
        return RESULT_ERROR_VULKAN;
    }

    // Create index buffer
    ctx->index_buffer = ARENA_PUSH_STRUCT(arena, VkBufferContext);
    result = vk_buffer_create_index(&ctx->device, ctx->command.pool,
                                    ctx->quad_mesh->indices,
                                    ctx->quad_mesh->index_count * sizeof(u32),
                                    ctx->index_buffer);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create index buffer");
        // Cleanup...
        return RESULT_ERROR_VULKAN;
    }

    LOG_INFO("Renderer initialized successfully");
    return RESULT_SUCCESS;
}

void renderer_shutdown(RendererContext *ctx) {
    LOG_INFO("Shutting down renderer");

    // Wait for device to be idle
    vk_device_wait_idle(&ctx->device);

    // Destroy geometry (Vulkan resources only, memory freed by arena)
    if (ctx->index_buffer) {
        vk_buffer_destroy(ctx->device.device, ctx->index_buffer);
    }
    if (ctx->vertex_buffer) {
        vk_buffer_destroy(ctx->device.device, ctx->vertex_buffer);
    }
    // Mesh data is arena-allocated, no need to call mesh_destroy

    // Destroy framebuffers (Vulkan handles only)
    destroy_framebuffers(ctx);

    // Destroy pipeline
    if (ctx->pipeline) {
        vk_pipeline_destroy(ctx->device.device, ctx->pipeline);
    }

    // Destroy render pass
    if (ctx->render_pass) {
        vk_renderpass_destroy(ctx->device.device, ctx->render_pass);
    }

    // Destroy command pool
    vk_command_destroy(ctx->device.device, &ctx->command);

    // Destroy sync objects
    vk_sync_destroy(ctx->device.device, &ctx->sync);

    // Destroy render-finished semaphores
    destroy_render_finished(ctx);

    // Destroy swapchain
    vk_swapchain_destroy(&ctx->device, &ctx->swapchain);

    // Destroy device
    vk_device_destroy(&ctx->device);

    // Destroy surface
    if (ctx->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(ctx->instance.instance, ctx->surface, NULL);
    }

    // Destroy instance
    vk_instance_destroy(&ctx->instance);

    LOG_INFO("Renderer shutdown complete");
}

void renderer_draw_frame(RendererContext *ctx) {
    // Wait for previous frame to finish
    vk_sync_wait_for_fence(ctx->device.device, &ctx->sync);

    // Acquire next image
    FrameSync *frame = vk_sync_get_current_frame(&ctx->sync);
    u32 image_index;

    VkResult result = vkAcquireNextImageKHR(ctx->device.device, ctx->swapchain.swapchain,
                                            UINT64_MAX, frame->image_available,
                                            VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        ctx->swapchain_needs_recreation = true;
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("Failed to acquire swapchain image: %d", result);
        return;
    }

    if (ctx->images_in_flight[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(ctx->device.device, 1, &ctx->images_in_flight[image_index], VK_TRUE, UINT64_MAX);
    }

    // Reset fence only when we're sure we're submitting work
    vk_sync_reset_fence(ctx->device.device, &ctx->sync);
    ctx->images_in_flight[image_index] = frame->in_flight;

    // Get command buffer
    VkCommandBuffer cmd = vk_command_get_buffer(&ctx->command, ctx->sync.current_frame);
    vk_command_begin(cmd);

    // Begin render pass
    VkClearValue clear_color = {{{0.1f, 0.1f, 0.15f, 1.0f}}};

    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = ctx->render_pass->render_pass,
        .framebuffer = ctx->framebuffers[image_index],
        .renderArea = {
            .offset = {0, 0},
            .extent = ctx->swapchain.extent,
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color,
    };

    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // Bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->pipeline->pipeline);

    // Set viewport and scissor
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)ctx->swapchain.extent.width,
        .height = (float)ctx->swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = ctx->swapchain.extent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind vertex buffer
    VkBuffer vertex_buffers[] = {ctx->vertex_buffer->buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertex_buffers, offsets);

    // Bind index buffer
    vkCmdBindIndexBuffer(cmd, ctx->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed
    vkCmdDrawIndexed(cmd, ctx->quad_mesh->index_count, 1, 0, 0, 0);

    // End render pass
    vkCmdEndRenderPass(cmd);

    // End command buffer
    vk_command_end(cmd);

    // Submit
    VkSemaphore wait_semaphores[] = {frame->image_available};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signal_semaphores[] = {ctx->render_finished[image_index]};

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

    result = vkQueueSubmit(ctx->device.graphics_queue, 1, &submit_info, frame->in_flight);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to submit draw command buffer: %d", result);
        return;
    }

    // Present
    VkSwapchainKHR swapchains[] = {ctx->swapchain.swapchain};

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = swapchains,
        .pImageIndices = &image_index,
    };

    result = vkQueuePresentKHR(ctx->device.present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        ctx->swapchain_needs_recreation = true;
    } else if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to present swapchain image: %d", result);
    }

    // Advance frame
    vk_sync_advance_frame(&ctx->sync);
}

void renderer_handle_resize(RendererContext *ctx, WindowContext *window) {
    u32 width, height;
    window_get_framebuffer_size(window, &width, &height);

    if (width == 0 || height == 0) {
        return;  // Minimized
    }

    vk_device_wait_idle(&ctx->device);
    recreate_swapchain(ctx, width, height);
    ctx->swapchain_needs_recreation = false;
}

void renderer_wait_idle(RendererContext *ctx) {
    vk_device_wait_idle(&ctx->device);
}

// Internal functions
static VkResult create_framebuffers(RendererContext *ctx, Arena *arena) {
    ctx->framebuffer_count = ctx->swapchain.image_count;
    ctx->framebuffers = ARENA_PUSH_ARRAY(arena, VkFramebuffer, ctx->framebuffer_count);

    for (u32 i = 0; i < ctx->framebuffer_count; i++) {
        VkImageView attachments[] = {ctx->swapchain.image_views[i]};

        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = ctx->render_pass->render_pass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = ctx->swapchain.extent.width,
            .height = ctx->swapchain.extent.height,
            .layers = 1,
        };

        VkResult result = vkCreateFramebuffer(ctx->device.device, &fb_info, NULL, &ctx->framebuffers[i]);
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create framebuffer %u: %d", i, result);
            return result;
        }
    }

    LOG_DEBUG("Created %u framebuffers", ctx->framebuffer_count);
    return VK_SUCCESS;
}

static void destroy_framebuffers(RendererContext *ctx) {
    if (ctx->framebuffers) {
        for (u32 i = 0; i < ctx->framebuffer_count; i++) {
            if (ctx->framebuffers[i] != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(ctx->device.device, ctx->framebuffers[i], NULL);
            }
        }
        // Memory freed by arena, just clear pointers
        ctx->framebuffers = NULL;
        ctx->framebuffer_count = 0;
    }
}

static VkResult recreate_swapchain(RendererContext *ctx, u32 width, u32 height) {
    LOG_INFO("Recreating swapchain (%ux%u)", width, height);

    // Destroy old Vulkan resources
    destroy_framebuffers(ctx);
    destroy_render_finished(ctx);

    // Recreate swapchain
    VkResult result = vk_swapchain_recreate(&ctx->device, ctx->surface, width, height, &ctx->swapchain);
    if (result != VK_SUCCESS) {
        return result;
    }

    memset(ctx->images_in_flight, 0, sizeof(ctx->images_in_flight));
    memset(ctx->render_finished, 0, sizeof(ctx->render_finished));

    result = create_render_finished(ctx);
    if (result != VK_SUCCESS) {
        return result;
    }

    // Recreate framebuffers (uses arena stored in ctx)
    return create_framebuffers(ctx, ctx->arena);
}

static VkResult create_render_finished(RendererContext *ctx) {
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    for (u32 i = 0; i < ctx->swapchain.image_count; i++) {
        VkResult result = vkCreateSemaphore(ctx->device.device, &semaphore_info, NULL, &ctx->render_finished[i]);
        if (result != VK_SUCCESS) {
            for (u32 j = 0; j < i; j++) {
                vkDestroySemaphore(ctx->device.device, ctx->render_finished[j], NULL);
                ctx->render_finished[j] = VK_NULL_HANDLE;
            }
            return result;
        }
    }

    return VK_SUCCESS;
}

static void destroy_render_finished(RendererContext *ctx) {
    for (u32 i = 0; i < ctx->swapchain.image_count; i++) {
        if (ctx->render_finished[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(ctx->device.device, ctx->render_finished[i], NULL);
            ctx->render_finished[i] = VK_NULL_HANDLE;
        }
    }
}
