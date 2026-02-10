#include "app.h"
#include "core/log.h"
#include "platform/input.h"
#include "renderer/renderer.h"
#include "utils/macros.h"

#include <SDL3/SDL_timer.h>

AppConfig app_config_default(void)
{
  return AppConfig{
    .name              = "Terrain Simulator",
    .window_width      = 1280,
    .window_height     = 720,
    .enable_validation = true,
  };
}

Result app_init(AppContext *app, const AppConfig *config)
{
  LOG_INFO("Initializing application: %s", config->name);

  app->config      = *config;
  app->state       = APP_STATE_UNINITIALIZED;
  app->renderer    = NULL;
  app->simulation  = SimulationState{};
  app->delta_time  = 0.0;
  app->total_time  = 0.0;
  app->frame_count = 0;

  // Initialize window system
  Result result = window_system_init();
  if(result != RESULT_SUCCESS) {
    LOG_ERROR("Failed to initialize window system");
    return result;
  }

  // Create window
  WindowConfig window_config = {
    .title      = config->name,
    .width      = config->window_width,
    .height     = config->window_height,
    .resizable  = true,
    .fullscreen = false,
  };

  result = window_create(&window_config, &app->window);
  if(result != RESULT_SUCCESS) {
    LOG_ERROR("Failed to create window");
    window_system_shutdown();
    return result;
  }

  // Initialize input
  input_init();
  input_attach_window(app->window.window_id);

  // Initialize memory arenas
  app->memory                = MemoryContext{};
  MemoryConfig memory_config = {
    .permanent_size = MEGABYTES(64),
    .transient_size = MEGABYTES(32),
    .frame_size     = MEGABYTES(16),
    .scratch_size   = MEGABYTES(16),
  };

  if(!memory_init(&app->memory, &memory_config)) {
    LOG_ERROR("Failed to initialize memory arenas");
    window_destroy(&app->window);
    window_system_shutdown();
    return RESULT_ERROR_OUT_OF_MEMORY;
  }

  result = simulation_init(
    &app->simulation, memory_arena(&app->memory, MEMORY_ARENA_PERMANENT));
  if(result != RESULT_SUCCESS) {
    LOG_ERROR("Failed to initialize simulation");
    memory_shutdown(&app->memory);
    window_destroy(&app->window);
    window_system_shutdown();
    return result;
  }

  RendererConfig renderer_config = {
    .app_name          = config->name,
    .enable_validation = config->enable_validation,
  };

  result = renderer_create(
    &app->renderer, &app->window, &renderer_config,
    memory_arena(&app->memory, MEMORY_ARENA_PERMANENT));
  if(result != RESULT_SUCCESS) {
    LOG_ERROR("Failed to initialize renderer");
    simulation_shutdown(&app->simulation);
    memory_shutdown(&app->memory);
    window_destroy(&app->window);
    window_system_shutdown();
    return result;
  }

  app->state = APP_STATE_RUNNING;
  LOG_INFO("Application initialized successfully");
  return RESULT_SUCCESS;
}

void app_shutdown(AppContext *app)
{
  LOG_INFO("Shutting down application");
  app->state = APP_STATE_SHUTTING_DOWN;

  if(app->renderer) {
    renderer_destroy(app->renderer);
    app->renderer = NULL;
  }

  simulation_shutdown(&app->simulation);

  // Destroy arenas (frees all app-lifetime allocations)
  memory_shutdown(&app->memory);

  window_destroy(&app->window);
  window_system_shutdown();

  LOG_INFO("Application shutdown complete");
}

void app_run(AppContext *app)
{
  LOG_INFO("Starting main loop");

  u64    last_ticks = SDL_GetTicksNS();
  Result result     = RESULT_SUCCESS;

  while(!window_should_close(&app->window)
        && app->state == APP_STATE_RUNNING) {
    // Calculate delta time
    u64 current_ticks = SDL_GetTicksNS();
    app->delta_time   = (f64)(current_ticks - last_ticks) / 1000000000.0;
    last_ticks        = current_ticks;
    app->total_time += app->delta_time;

    // Update input
    input_update();

    // Reset per-frame arena
    memory_begin_frame(&app->memory);

    // Poll events
    SDL_Event event;
    while(window_poll_event(&app->window, &event)) {
      input_handle_event(&event);
    }

    // Handle escape key
    if(input_key_pressed(KEY_ESCAPE)) {
      app_request_shutdown(app);
      continue;
    }

    // Skip rendering if minimized
    if(app->window.minimized) {
      if(window_wait_event(&app->window, &event)) {
        input_handle_event(&event);
        while(window_poll_event(&app->window, &event)) {
          input_handle_event(&event);
        }
      }
      input_reset_scroll();
      continue;
    }

    // Handle window resize
    if(app->window.resized) {
      result = renderer_resize(app->renderer);
      window_reset_resized(&app->window);
      if(result != RESULT_SUCCESS) {
        LOG_ERROR("Renderer resize failed: %d", result);
        app_request_shutdown(app);
        continue;
      }
    }

    simulation_update(&app->simulation, app->delta_time);

    // Render frame
    result = renderer_draw(app->renderer);
    if(result != RESULT_SUCCESS) {
      LOG_ERROR("Renderer frame failed: %d", result);
      app_request_shutdown(app);
      continue;
    }

    app->frame_count++;

    // Reset scroll
    input_reset_scroll();
  }

  // Wait for device idle before shutdown
  if(app->renderer) {
    renderer_wait_idle(app->renderer);
  }

  LOG_INFO("Main loop ended after %llu frames", app->frame_count);
}

void app_request_shutdown(AppContext *app)
{
  LOG_INFO("Shutdown requested");
  app->state = APP_STATE_SHUTTING_DOWN;
}

bool app_is_running(AppContext *app)
{
  return app->state == APP_STATE_RUNNING;
}
