#pragma once

#include "SDL3/SDL_mouse.h"
#include "glass.h"
#include <SDL3/SDL.h>

struct KeyState {
    bool hold;
};

union InputDevice {
    InputDeviceType type;

    struct {
        float       mouse_delta[2];
        float       mouse_scroll_delta[2];
        KeyState*   mouse_buttons;
        SDL_MouseID mouse_id;
    };
};

struct Window {
    SDL_Window* window;
    u32         sdl_id;
    KeyState*   keys;
    bool        should_close;
};
