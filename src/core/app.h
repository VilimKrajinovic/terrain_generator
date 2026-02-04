#ifndef APP_H
#define APP_H

#include "memory/memory.h"
#include "platform/window.h"
#include "utils/types.h"

// Forward declarations
typedef struct RendererContext RendererContext;

// Application state
typedef enum AppState {
  APP_STATE_UNINITIALIZED = 0,
  APP_STATE_RUNNING,
  APP_STATE_PAUSED,
  APP_STATE_SHUTTING_DOWN,
} AppState;

// Application configuration
typedef struct AppConfig {
  const char *name;
  u32 window_width;
  u32 window_height;
  bool enable_validation;
} AppConfig;

// Application context
typedef struct AppContext {
  AppConfig config;
  AppState state;
  MemoryContext memory;
  WindowContext window;
  RendererContext *renderer;
  f64 delta_time;
  f64 total_time;
  u64 frame_count;
} AppContext;

// Create default app config
AppConfig app_config_default(void);

// Initialize application
Result app_init(AppContext *app, const AppConfig *config);

// Shutdown application
void app_shutdown(AppContext *app);

// Run main loop
void app_run(AppContext *app);

// Request application shutdown
void app_request_shutdown(AppContext *app);

// Check if application is running
bool app_is_running(AppContext *app);

#endif // APP_H
