#include "input.h"
#include "core/log.h"

#include <SDL3/SDL.h>
#include <string.h>

typedef struct InputState {
    bool keys[SDL_SCANCODE_COUNT];
    bool keys_previous[SDL_SCANCODE_COUNT];
    bool mouse_buttons[8];
    bool mouse_buttons_previous[8];
    f64 mouse_x;
    f64 mouse_y;
    f64 mouse_dx;
    f64 mouse_dy;
    f64 scroll_x;
    f64 scroll_y;
} InputState;

static InputState g_input = {0};
static u32 g_window_id = 0;

static bool input_event_matches_window(const SDL_Event *event) {
    if (!g_window_id) {
        return true;
    }

    switch (event->type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            return event->key.windowID == g_window_id;
        case SDL_EVENT_MOUSE_MOTION:
            return event->motion.windowID == g_window_id;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return event->button.windowID == g_window_id;
        case SDL_EVENT_MOUSE_WHEEL:
            return event->wheel.windowID == g_window_id;
        default:
            return true;
    }
}

static i32 mouse_button_to_index(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return MOUSE_BUTTON_LEFT;
        case SDL_BUTTON_MIDDLE:
            return MOUSE_BUTTON_MIDDLE;
        case SDL_BUTTON_RIGHT:
            return MOUSE_BUTTON_RIGHT;
        default:
            return -1;
    }
}

void input_init(void) {
    memset(&g_input, 0, sizeof(g_input));
    g_window_id = 0;

    float mx = 0.0f;
    float my = 0.0f;
    SDL_GetMouseState(&mx, &my);
    g_input.mouse_x = (f64)mx;
    g_input.mouse_y = (f64)my;

    LOG_INFO("Input system initialized");
}

void input_attach_window(u32 window_id) {
    g_window_id = window_id;
}

void input_update(void) {
    // Store previous state
    memcpy(g_input.keys_previous, g_input.keys, sizeof(g_input.keys));
    memcpy(g_input.mouse_buttons_previous, g_input.mouse_buttons, sizeof(g_input.mouse_buttons));

    // Reset delta
    g_input.mouse_dx = 0.0;
    g_input.mouse_dy = 0.0;
}

void input_handle_event(const SDL_Event *event) {
    if (!input_event_matches_window(event)) {
        return;
    }

    switch (event->type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            SDL_Scancode scancode = event->key.scancode;
            if (scancode >= 0 && scancode < SDL_SCANCODE_COUNT) {
                g_input.keys[scancode] = event->key.down;
            }
        } break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            i32 index = mouse_button_to_index(event->button.button);
            if (index >= 0 && index < 8) {
                g_input.mouse_buttons[index] = event->button.down;
            }
        } break;
        case SDL_EVENT_MOUSE_MOTION:
            g_input.mouse_dx = (f64)event->motion.xrel;
            g_input.mouse_dy = (f64)event->motion.yrel;
            g_input.mouse_x = (f64)event->motion.x;
            g_input.mouse_y = (f64)event->motion.y;
            break;
        case SDL_EVENT_MOUSE_WHEEL: {
            f32 x = event->wheel.x;
            f32 y = event->wheel.y;
            if (event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                x = -x;
                y = -y;
            }
            g_input.scroll_x += (f64)x;
            g_input.scroll_y += (f64)y;
        } break;
        default:
            break;
    }
}

bool input_key_down(KeyCode key) {
    return g_input.keys[key];
}

bool input_key_pressed(KeyCode key) {
    return g_input.keys[key] && !g_input.keys_previous[key];
}

bool input_key_released(KeyCode key) {
    return !g_input.keys[key] && g_input.keys_previous[key];
}

bool input_mouse_down(MouseButton button) {
    return g_input.mouse_buttons[button];
}

bool input_mouse_pressed(MouseButton button) {
    return g_input.mouse_buttons[button] && !g_input.mouse_buttons_previous[button];
}

bool input_mouse_released(MouseButton button) {
    return !g_input.mouse_buttons[button] && g_input.mouse_buttons_previous[button];
}

void input_get_mouse_position(f64 *x, f64 *y) {
    if (x) *x = g_input.mouse_x;
    if (y) *y = g_input.mouse_y;
}

void input_get_mouse_delta(f64 *dx, f64 *dy) {
    if (dx) *dx = g_input.mouse_dx;
    if (dy) *dy = g_input.mouse_dy;
}

void input_get_scroll(f64 *x, f64 *y) {
    if (x) *x = g_input.scroll_x;
    if (y) *y = g_input.scroll_y;
}

void input_reset_scroll(void) {
    g_input.scroll_x = 0.0;
    g_input.scroll_y = 0.0;
}
