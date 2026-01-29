#include "window.h"
#include "core/log.h"
#include "platform/input.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

static void window_update_size(WindowContext *ctx) {
  int width = 0;
  int height = 0;
  SDL_GetWindowSizeInPixels(ctx->handle, &width, &height);
  ctx->width = (u32)width;
  ctx->height = (u32)height;
  ctx->minimized = (width == 0 || height == 0);
}

static void window_handle_event(WindowContext *ctx, const SDL_Event *event) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
    ctx->should_close = true;
    break;
  case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    if (event->window.windowID == ctx->window_id) {
      ctx->should_close = true;
    }
    break;
  case SDL_EVENT_WINDOW_MINIMIZED:
    if (event->window.windowID == ctx->window_id) {
      ctx->minimized = true;
    }
    break;
  case SDL_EVENT_WINDOW_RESTORED:
    if (event->window.windowID == ctx->window_id) {
      ctx->minimized = false;
    }
    break;
  case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
  case SDL_EVENT_WINDOW_RESIZED:
    if (event->window.windowID == ctx->window_id) {
      window_update_size(ctx);
      ctx->resized = true;
      LOG_DEBUG("Window resized to %ux%u", ctx->width, ctx->height);
    }
    break;
  default:
    break;
  }

  input_handle_event(event);
}

WindowConfig window_config_default(void) {
  return (WindowConfig){
      .title = "Terrain Simulator",
      .width = 1280,
      .height = 720,
      .resizable = true,
      .fullscreen = false,
  };
}

Result window_system_init(void) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LOG_ERROR("Failed to initialize SDL: %s", SDL_GetError());
    return RESULT_ERROR_WINDOW;
  }

  LOG_INFO("SDL initialized successfully");
  return RESULT_SUCCESS;
}

void window_system_shutdown(void) {
  SDL_Quit();
  LOG_INFO("SDL shut down");
}

Result window_create(const WindowConfig *config, WindowContext *ctx) {
  Uint32 flags = SDL_WINDOW_VULKAN;
  if (config->resizable) {
    flags |= SDL_WINDOW_RESIZABLE;
  }
  if (config->fullscreen) {
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  ctx->handle = SDL_CreateWindow(config->title, (int)config->width,
                                 (int)config->height, flags);
  if (!ctx->handle) {
    LOG_ERROR("Failed to create SDL window: %s", SDL_GetError());
    return RESULT_ERROR_WINDOW;
  }

  ctx->window_id = (u32)SDL_GetWindowID(ctx->handle);
  ctx->width = config->width;
  ctx->height = config->height;
  window_update_size(ctx);
  ctx->resized = false;
  ctx->minimized = false;
  ctx->should_close = false;

  LOG_INFO("Window created: %s (%ux%u)", config->title, ctx->width,
           ctx->height);
  return RESULT_SUCCESS;
}

void window_destroy(WindowContext *ctx) {
  if (ctx->handle) {
    SDL_DestroyWindow(ctx->handle);
    ctx->handle = NULL;
    ctx->should_close = true;
    LOG_INFO("Window destroyed");
  }
}

bool window_should_close(WindowContext *ctx) { return ctx->should_close; }

void window_poll_events(WindowContext *ctx) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    window_handle_event(ctx, &event);
  }
}

void window_wait_events(WindowContext *ctx) {
  SDL_Event event;
  if (SDL_WaitEvent(&event)) {
    window_handle_event(ctx, &event);
  }

  while (SDL_PollEvent(&event)) {
    window_handle_event(ctx, &event);
  }
}

void window_get_framebuffer_size(WindowContext *ctx, u32 *width, u32 *height) {
  int w, h;
  SDL_GetWindowSizeInPixels(ctx->handle, &w, &h);
  if (width) {
    *width = (u32)w;
  }
  if (height) {
    *height = (u32)h;
  }
}

VkResult window_create_surface(WindowContext *ctx, VkInstance instance,
                               VkSurfaceKHR *surface) {
  if (!SDL_Vulkan_CreateSurface(ctx->handle, instance, NULL, surface)) {
    LOG_ERROR("Failed to create Vulkan surface: %s", SDL_GetError());
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  LOG_INFO("Vulkan surface created");
  return VK_SUCCESS;
}

const char **window_get_required_extensions(u32 *count) {
  Uint32 sdl_count = 0;
  const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_count);
  if (!extensions) {
    LOG_ERROR("Failed to get Vulkan instance extensions: %s", SDL_GetError());
    if (count) {
      *count = 0;
    }
    return NULL;
  }

  if (count) {
    *count = (u32)sdl_count;
  }

  return (const char **)extensions;
}

void window_reset_resized(WindowContext *ctx) { ctx->resized = false; }

void window_set_user_pointer(WindowContext *ctx, void *user_data) {
  SDL_SetPointerProperty(SDL_GetWindowProperties(ctx->handle), "user_ptr",
                         user_data);
}
