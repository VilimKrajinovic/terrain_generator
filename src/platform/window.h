#ifndef WINDOW_H
#define WINDOW_H

#include "utils/types.h"
#include <vulkan/vulkan.h>

// Forward declaration
struct SDL_Window;

// Window configuration
typedef struct WindowConfig {
    const char *title;
    u32 width;
    u32 height;
    bool resizable;
    bool fullscreen;
} WindowConfig;

// Window context
typedef struct WindowContext {
    struct SDL_Window *handle;
    u32 window_id;
    u32 width;
    u32 height;
    bool resized;
    bool minimized;
    bool should_close;
} WindowContext;

// Create default window config
WindowConfig window_config_default(void);

// Initialize window system (call once at startup)
Result window_system_init(void);

// Shutdown window system (call once at shutdown)
void window_system_shutdown(void);

// Create a window
Result window_create(const WindowConfig *config, WindowContext *ctx);

// Destroy a window
void window_destroy(WindowContext *ctx);

// Check if window should close
bool window_should_close(WindowContext *ctx);

// Poll window events
void window_poll_events(WindowContext *ctx);

// Wait for events (blocks)
void window_wait_events(WindowContext *ctx);

// Get framebuffer size (may differ from window size on HiDPI)
void window_get_framebuffer_size(WindowContext *ctx, u32 *width, u32 *height);

// Create Vulkan surface for window
VkResult window_create_surface(WindowContext *ctx, VkInstance instance, VkSurfaceKHR *surface);

// Get required Vulkan extensions for window surface
const char **window_get_required_extensions(u32 *count);

// Reset resized flag
void window_reset_resized(WindowContext *ctx);

// Set window resize callback user data
void window_set_user_pointer(WindowContext *ctx, void *user_data);

#endif // WINDOW_H
