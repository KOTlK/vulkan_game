#pragma once

#include "glass.h"
#include <SDL3/SDL.h>

struct KeyState {
    bool hold;
};

struct Window {
    SDL_Window* window;
    u32         sdl_id;
    KeyState*   keys;
    bool        should_close;
};
