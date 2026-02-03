#include "app.h"
#include "core/log.h"
#include "platform/input.h"
#include "renderer/renderer.h"
#include "utils/macros.h"

#include <SDL3/SDL_timer.h>

AppConfig app_config_default(void) {
  return (AppConfig){
      .name = "Terrain Simulator",
      .window_width = 1280,
      .window_height = 720,
      .enable_validation = true,
  };
}

Result app_init(AppContext *app, const AppConfig *config) {
  LOG_INFO("Initializing application: %s", config->name);

  app->config = *config;
  app->state = APP_STATE_UNINITIALIZED;
  app->delta_time = 0.0;
  app->total_time = 0.0;
  app->frame_count = 0;

  // Initialize window system
  Result result = window_system_init();
  if (result != RESULT_SUCCESS) {
    LOG_ERROR("Failed to initialize window system");
    return result;
  }

  // Create window
  WindowConfig window_config = {
      .title = config->name,
      .width = config->window_width,
      .height = config->window_height,
      .resizable = true,
      .fullscreen = false,
  };

  result = window_create(&window_config, &app->window);
  if (result != RESULT_SUCCESS) {
    LOG_ERROR("Failed to create window");
    window_system_shutdown();
    return result;
  }

  // Initialize input
  input_init(&app->window);

  // Create permanent arena
  app->permanent_arena = arena_create(MEGABYTES(64));
  if (!app->permanent_arena.base) {
    LOG_ERROR("Failed to create permanent arena");
    window_destroy(&app->window);
    window_system_shutdown();
    return RESULT_ERROR_OUT_OF_MEMORY;
  }

  // Initialize renderer
  app->renderer = ARENA_PUSH_STRUCT(&app->permanent_arena, RendererContext);
  if (!app->renderer) {
    LOG_ERROR("Failed to allocate renderer context");
    arena_destroy(&app->permanent_arena);
    window_destroy(&app->window);
    window_system_shutdown();
    return RESULT_ERROR_OUT_OF_MEMORY;
  }

  RendererConfig renderer_config = {
      .app_name = config->name,
      .enable_validation = config->enable_validation,
  };

  result = renderer_init(app->renderer, &app->window, &renderer_config,
                         &app->permanent_arena);
  if (result != RESULT_SUCCESS) {
    LOG_ERROR("Failed to initialize renderer");
    arena_destroy(&app->permanent_arena);
    window_destroy(&app->window);
    window_system_shutdown();
    return result;
  }

  app->state = APP_STATE_RUNNING;
  LOG_INFO("Application initialized successfully");
  return RESULT_SUCCESS;
}

void app_shutdown(AppContext *app) {
  LOG_INFO("Shutting down application");
  app->state = APP_STATE_SHUTTING_DOWN;

  if (app->renderer) {
    renderer_shutdown(app->renderer);
    app->renderer = NULL;
  }

  // Destroy permanent arena (frees all app-lifetime allocations)
  arena_destroy(&app->permanent_arena);

  window_destroy(&app->window);
  window_system_shutdown();

  LOG_INFO("Application shutdown complete");
}

void app_run(AppContext *app) {
  LOG_INFO("Starting main loop");

  u64 last_ticks = SDL_GetTicksNS();

  while (!window_should_close(&app->window) &&
         app->state == APP_STATE_RUNNING) {
    // Calculate delta time
    u64 current_ticks = SDL_GetTicksNS();
    app->delta_time = (f64)(current_ticks - last_ticks) / 1000000000.0;
    last_ticks = current_ticks;
    app->total_time += app->delta_time;

    // Update input
    input_update();

    // Poll events
    window_poll_events(&app->window);

    // Handle escape key
    if (input_key_pressed(KEY_ESCAPE)) {
      app_request_shutdown(app);
      continue;
    }

    // Skip rendering if minimized
    if (app->window.minimized) {
      window_wait_events(&app->window);
      continue;
    }

    // Handle window resize
    if (app->window.resized) {
      renderer_handle_resize(app->renderer, &app->window);
      window_reset_resized(&app->window);
    }

    // Render frame
    renderer_draw_frame(app->renderer);

    app->frame_count++;

    // Reset scroll
    input_reset_scroll();
  }

  // Wait for device idle before shutdown
  renderer_wait_idle(app->renderer);

  LOG_INFO("Main loop ended after %llu frames", app->frame_count);
}

void app_request_shutdown(AppContext *app) {
  LOG_INFO("Shutdown requested");
  app->state = APP_STATE_SHUTTING_DOWN;
}

bool app_is_running(AppContext *app) { return app->state == APP_STATE_RUNNING; }
