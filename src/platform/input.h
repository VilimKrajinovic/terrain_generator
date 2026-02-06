#ifndef INPUT_H
#define INPUT_H

#include "utils/types.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>

// Key codes (SDL scancodes)
typedef enum KeyCode {
    KEY_UNKNOWN = SDL_SCANCODE_UNKNOWN,
    KEY_SPACE = SDL_SCANCODE_SPACE,
    KEY_ESCAPE = SDL_SCANCODE_ESCAPE,
    KEY_ENTER = SDL_SCANCODE_RETURN,
    KEY_TAB = SDL_SCANCODE_TAB,
    KEY_BACKSPACE = SDL_SCANCODE_BACKSPACE,
    KEY_RIGHT = SDL_SCANCODE_RIGHT,
    KEY_LEFT = SDL_SCANCODE_LEFT,
    KEY_DOWN = SDL_SCANCODE_DOWN,
    KEY_UP = SDL_SCANCODE_UP,
    KEY_F1 = SDL_SCANCODE_F1,
    KEY_F2 = SDL_SCANCODE_F2,
    KEY_F3 = SDL_SCANCODE_F3,
    KEY_F11 = SDL_SCANCODE_F11,
    KEY_F12 = SDL_SCANCODE_F12,
    KEY_LEFT_SHIFT = SDL_SCANCODE_LSHIFT,
    KEY_LEFT_CONTROL = SDL_SCANCODE_LCTRL,
    KEY_LEFT_ALT = SDL_SCANCODE_LALT,
    KEY_W = SDL_SCANCODE_W,
    KEY_A = SDL_SCANCODE_A,
    KEY_S = SDL_SCANCODE_S,
    KEY_D = SDL_SCANCODE_D,
    KEY_Q = SDL_SCANCODE_Q,
    KEY_E = SDL_SCANCODE_E,
    KEY_R = SDL_SCANCODE_R,
} KeyCode;

// Mouse button codes
typedef enum MouseButton {
    MOUSE_BUTTON_LEFT = SDL_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE = SDL_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT = SDL_BUTTON_RIGHT,
} MouseButton;

// Initialize input system
void input_init(void);

// Attach input to a window id (0 means all windows)
void input_attach_window(u32 window_id);

// Update input state (call once per frame before polling events)
void input_update(void);

// Handle SDL events (called from window event pump)
void input_handle_event(const SDL_Event *event);

// Key state queries
bool input_key_down(KeyCode key);
bool input_key_pressed(KeyCode key);
bool input_key_released(KeyCode key);

// Mouse button state queries
bool input_mouse_down(MouseButton button);
bool input_mouse_pressed(MouseButton button);
bool input_mouse_released(MouseButton button);

// Mouse position
void input_get_mouse_position(f64 *x, f64 *y);
void input_get_mouse_delta(f64 *dx, f64 *dy);

// Scroll
void input_get_scroll(f64 *x, f64 *y);

// Reset scroll accumulator
void input_reset_scroll(void);

#endif // INPUT_H
