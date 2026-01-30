#ifndef RENDERER_H
#define RENDERER_H

#include "utils/types.h"
#include "utils/arena.h"
#include "platform/window.h"
#include "renderer/vk_instance.h"
#include "renderer/vk_device.h"
#include "renderer/vk_swapchain.h"
#include "renderer/vk_sync.h"
#include "renderer/vk_command.h"

// Forward declarations
typedef struct VkRenderPassContext VkRenderPassContext;
typedef struct VkPipelineContext VkPipelineContext;
typedef struct VkBufferContext VkBufferContext;
typedef struct Mesh Mesh;

// Renderer configuration
typedef struct RendererConfig {
    const char *app_name;
    bool enable_validation;
} RendererConfig;

// Renderer context - holds all Vulkan state
typedef struct RendererContext {
    // Core Vulkan objects
    VkInstanceContext instance;
    VkSurfaceKHR surface;
    VkDeviceContext device;
    VkSwapchainContext swapchain;
    VkSyncContext sync;
    VkCommandContext command;

    // Render pass and pipeline
    VkRenderPassContext *render_pass;
    VkPipelineContext *pipeline;

    // Framebuffers
    VkFramebuffer *framebuffers;
    u32 framebuffer_count;
    VkFence images_in_flight[MAX_SWAPCHAIN_IMAGES];
    VkSemaphore render_finished[MAX_SWAPCHAIN_IMAGES];

    // Geometry
    VkBufferContext *vertex_buffer;
    VkBufferContext *index_buffer;
    Mesh *quad_mesh;

    // Memory
    Arena *arena;

    // State
    bool swapchain_needs_recreation;
} RendererContext;

// Initialize renderer
Result renderer_init(RendererContext *ctx, WindowContext *window, const RendererConfig *config, Arena *arena);

// Shutdown renderer
void renderer_shutdown(RendererContext *ctx);

// Draw a frame
void renderer_draw_frame(RendererContext *ctx);

// Handle window resize
void renderer_handle_resize(RendererContext *ctx, WindowContext *window);

// Wait for device idle
void renderer_wait_idle(RendererContext *ctx);

#endif // RENDERER_H
