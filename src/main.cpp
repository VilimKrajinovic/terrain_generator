#include "core/app.h"
#include "core/log.h"
#include "utils/macros.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
  UNUSED(argc);
  UNUSED(argv);

  // Initialize logging
  log_init(LOG_LEVEL_DEBUG);
  LOG_INFO("=== Terrain Simulator ===");

  // Create application
  AppContext app   = {};
  AppConfig config = app_config_default();

  Result result = app_init(&app, &config);
  if(result != RESULT_SUCCESS) {
    LOG_ERROR("Failed to initialize application: %d", result);
    log_shutdown();
    return EXIT_FAILURE;
  }

  // Run main loop
  app_run(&app);

  // Cleanup
  app_shutdown(&app);
  log_shutdown();

  return EXIT_SUCCESS;
}
