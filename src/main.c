#include "core/app.h"
#include "core/log.h"
#include "memory/arena.h"
#include "utils/macros.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    // Initialize logging
    log_init(LOG_LEVEL_DEBUG);

    LOG_INFO("=== Terrain Simulator ===");

    // Initialize scratch arena for temporary allocations
    arena_scratch_init(MEGABYTES(16));

    // Create application
    AppContext app = {0};
    AppConfig config = app_config_default();

    Result result = app_init(&app, &config);
    if (result != RESULT_SUCCESS) {
        LOG_ERROR("Failed to initialize application: %d", result);
        arena_scratch_shutdown();
        log_shutdown();
        return EXIT_FAILURE;
    }

    // Run main loop
    app_run(&app);

    // Cleanup
    app_shutdown(&app);
    arena_scratch_shutdown();
    log_shutdown();

    return EXIT_SUCCESS;
}
