#include "SDL3/SDL_mouse.h"
#include "glass.h"
#include "list.h"
#include "queue.h"
#include "hash_table.h"
#include "glass_sdl.h"
#include <stdio.h>
#include "debug.h"

#define Calloc(type, count) (type*)malloc(sizeof(type) * count)

static List<Window>            Windows;
static Queue<u32>              Empty_Windows;
static HashTable<u32, Window*> Window_By_Id;
static bool                    Initialized = false;
static bool                    Should_Quit = false;

static inline Window* get_window(u32 id);
static inline void dispatch_event(SDL_Event event);
static inline void push_button(Window* win, GlassScancode scancode);
static inline void release_button(Window* win, GlassScancode scancode);
static inline GlassScancode sdl_scancode_to_glass(SDL_Scancode scancode);

Window* glass_create_window(u32 x, u32 y, u32 width, u32 height, const char* name, GlassErrorCode* err) {
    if (!Initialized) {
        list_make(&Windows);
        queue_make(&Empty_Windows);
        table_make(&Window_By_Id);
        Initialized = true;
    }

    SDL_Window* sdl_window = SDL_CreateWindow(
        name,
        width, height,
        SDL_WINDOW_OPENGL | 
        SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_INPUT_FOCUS
    );

    if (sdl_window == NULL) {
        Errf("%s", SDL_GetError());
        *err = GLASS_INTERNAL_ERROR;
        return NULL;
    }

    u32 id = SDL_GetWindowID(sdl_window);

    Window* window;

    if (Empty_Windows.count > 0) {
        u32 index = queue_dequeue(&Empty_Windows);
        window = list_get_ptr(&Windows, index);
    } else {
        window = list_append_empty(&Windows);
    }

    window->window = sdl_window;
    window->sdl_id = id;
    window->keys   = Calloc(KeyState, SCANCODE_COUNT);

    memset(window->keys, 0, sizeof(KeyState) * SCANCODE_COUNT);

    table_add(&Window_By_Id, id, window);

    SDL_SetWindowPosition(sdl_window, x, y);

    *err = GLASS_OK;

    return window;
}

void glass_destroy_window(Window* window) {
    u32 index      = list_index_of_ptr(&Windows, window);
    queue_enqueue(&Empty_Windows, index);
    table_remove(&Window_By_Id, window->sdl_id);
    SDL_DestroyWindow(window->window);
    Windows[index] = {};

    if (Windows.count == 0) {
        Should_Quit = true;
    }
}

void glass_destroy_all_windows() {
    for (Window& window : Windows) {
        glass_destroy_window(&window);
    }
}

void glass_set_window_title(Window* window, const char* title) {
    SDL_SetWindowTitle(window->window, title);
}

const char* glass_get_executable_path() {
    return SDL_GetBasePath();
}

// void glass_exit() {
    // for (Window& window : Windows) {
    //     glass_destroy_window(&window);
    // }
// }

bool glass_exit_required() {
    return Should_Quit;
}

void glass_main_loop() {
    SDL_Event event;

    while(SDL_PollEvent(&event)) {
        dispatch_event(event);
    }

    glass_game_code();

    for (Window& window : Windows) {
        if (window.should_close) {
            glass_destroy_window(&window);
            return;
        }

        GlassErrorCode err = glass_render(&window);

        if (err != GLASS_OK) {
            glass_exit();
            return;
        }

        err = glass_swap_buffers(&window);

        if (err != GLASS_OK) {
            glass_exit();
            return;
        }
    }
}

bool glass_is_button_pressed(Window* win, GlassScancode sc) {
    return win->keys[sc].hold;
}

u64 glass_query_performance_counter() {
    return SDL_GetPerformanceCounter();
}

u64 glass_query_performance_frequency() {
    return SDL_GetPerformanceFrequency();
}

void glass_sleep(u64 time) {
    SDL_Delay(time);
}

GlassErrorCode glass_swap_buffers(Window* window) {
#ifdef GLASS_OPENGL
    SDL_GL_SwapWindow(window->window);
#endif
    return GLASS_OK;
}

static inline Window* get_window(u32 id) {
    return table_get(&Window_By_Id, id);
}

static inline void dispatch_event(SDL_Event event) {
    switch(event.type) {
        case SDL_EVENT_QUIT:
            Should_Quit = true;
        break;
        case SDL_EVENT_KEY_DOWN: {
            Window* win = get_window(event.window.windowID);
            push_button(win, sdl_scancode_to_glass(event.key.scancode));
        } break;
        case SDL_EVENT_KEY_UP: {
            Window* win = get_window(event.window.windowID);
            release_button(win, sdl_scancode_to_glass(event.key.scancode));
        } break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            Window* win = get_window(event.window.windowID);

            switch (event.button.button) {
                case SDL_BUTTON_LEFT  : push_button(win, GLASS_SCANCODE_MOUSE0); break;
                case SDL_BUTTON_RIGHT : push_button(win, GLASS_SCANCODE_MOUSE1); break;
                case SDL_BUTTON_MIDDLE: push_button(win, GLASS_SCANCODE_MOUSE2); break;
                case SDL_BUTTON_X1    : push_button(win, GLASS_SCANCODE_MOUSE3); break;
                case SDL_BUTTON_X2    : push_button(win, GLASS_SCANCODE_MOUSE4); break;
            }
        } break;
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            Window* win = get_window(event.window.windowID);

            switch (event.button.button) {
                case SDL_BUTTON_LEFT  : release_button(win, GLASS_SCANCODE_MOUSE0); break;
                case SDL_BUTTON_RIGHT : release_button(win, GLASS_SCANCODE_MOUSE1); break;
                case SDL_BUTTON_MIDDLE: release_button(win, GLASS_SCANCODE_MOUSE2); break;
                case SDL_BUTTON_X1    : release_button(win, GLASS_SCANCODE_MOUSE3); break;
                case SDL_BUTTON_X2    : release_button(win, GLASS_SCANCODE_MOUSE4); break;
            }
        } break;
        case SDL_EVENT_WINDOW_RESIZED: {
            glass_on_resize(event.window.data1, event.window.data2);
            // Logf("Resized. %i, %i.", event.window.data1, event.window.data2);
        } break;
        case SDL_EVENT_WINDOW_MOVED: {
            glass_on_move(event.window.data1, event.window.data2);
            // Logf("Resized. %i, %i.", event.window.data1, event.window.data2);
        } break;
        default:
        break;
    }
}

static inline void push_button(Window* win, GlassScancode scancode) {
    win->keys[scancode].hold = true;
}

static inline void release_button(Window* win, GlassScancode scancode) {
    win->keys[scancode].hold = false;
}

static inline GlassScancode sdl_scancode_to_glass(SDL_Scancode scancode) {
    switch(scancode) {
        case SDL_SCANCODE_UNKNOWN     : return GLASS_SCANCODE_UNASSIGNED;
        case SDL_SCANCODE_TAB         : return GLASS_SCANCODE_TAB;
        case SDL_SCANCODE_BACKSPACE   : return GLASS_SCANCODE_BACKSPACE;
        case SDL_SCANCODE_RETURN      : return GLASS_SCANCODE_RETURN;
        case SDL_SCANCODE_LSHIFT      : return GLASS_SCANCODE_LEFT_SHIFT;
        case SDL_SCANCODE_RSHIFT      : return GLASS_SCANCODE_RIGHT_SHIFT;
        case SDL_SCANCODE_LCTRL       : return GLASS_SCANCODE_LEFT_CONTROL;
        case SDL_SCANCODE_RCTRL       : return GLASS_SCANCODE_RIGHT_CONTROL;
        case SDL_SCANCODE_LALT        : return GLASS_SCANCODE_LEFT_ALT;
        case SDL_SCANCODE_RALT        : return GLASS_SCANCODE_RIGHT_ALT;
        case SDL_SCANCODE_PAUSE       : return GLASS_SCANCODE_PAUSE;
        case SDL_SCANCODE_CAPSLOCK    : return GLASS_SCANCODE_CAPS;
        case SDL_SCANCODE_ESCAPE      : return GLASS_SCANCODE_ESCAPE;
        case SDL_SCANCODE_SPACE       : return GLASS_SCANCODE_SPACE;
        case SDL_SCANCODE_PAGEUP      : return GLASS_SCANCODE_PAGEUP;
        case SDL_SCANCODE_PAGEDOWN    : return GLASS_SCANCODE_PAGEDOWN;
        case SDL_SCANCODE_END         : return GLASS_SCANCODE_END;
        case SDL_SCANCODE_HOME        : return GLASS_SCANCODE_HOME;
        case SDL_SCANCODE_LEFT        : return GLASS_SCANCODE_LEFT;
        case SDL_SCANCODE_UP          : return GLASS_SCANCODE_UP;
        case SDL_SCANCODE_RIGHT       : return GLASS_SCANCODE_RIGHT;
        case SDL_SCANCODE_DOWN        : return GLASS_SCANCODE_DOWN;
        case SDL_SCANCODE_PRINTSCREEN : return GLASS_SCANCODE_PRINTSCREEN;
        case SDL_SCANCODE_INSERT      : return GLASS_SCANCODE_INSERT;
        case SDL_SCANCODE_DELETE      : return GLASS_SCANCODE_DELETE;
        case SDL_SCANCODE_0           : return GLASS_SCANCODE_ALPHA0;
        case SDL_SCANCODE_1           : return GLASS_SCANCODE_ALPHA1;
        case SDL_SCANCODE_2           : return GLASS_SCANCODE_ALPHA2;
        case SDL_SCANCODE_3           : return GLASS_SCANCODE_ALPHA3;
        case SDL_SCANCODE_4           : return GLASS_SCANCODE_ALPHA4;
        case SDL_SCANCODE_5           : return GLASS_SCANCODE_ALPHA5;
        case SDL_SCANCODE_6           : return GLASS_SCANCODE_ALPHA6;
        case SDL_SCANCODE_7           : return GLASS_SCANCODE_ALPHA7;
        case SDL_SCANCODE_8           : return GLASS_SCANCODE_ALPHA8;
        case SDL_SCANCODE_9           : return GLASS_SCANCODE_ALPHA9;
        case SDL_SCANCODE_A           : return GLASS_SCANCODE_A;
        case SDL_SCANCODE_B           : return GLASS_SCANCODE_B;
        case SDL_SCANCODE_C           : return GLASS_SCANCODE_C;
        case SDL_SCANCODE_D           : return GLASS_SCANCODE_D;
        case SDL_SCANCODE_E           : return GLASS_SCANCODE_E;
        case SDL_SCANCODE_F           : return GLASS_SCANCODE_F;
        case SDL_SCANCODE_G           : return GLASS_SCANCODE_G;
        case SDL_SCANCODE_H           : return GLASS_SCANCODE_H;
        case SDL_SCANCODE_I           : return GLASS_SCANCODE_I;
        case SDL_SCANCODE_J           : return GLASS_SCANCODE_J;
        case SDL_SCANCODE_K           : return GLASS_SCANCODE_K;
        case SDL_SCANCODE_L           : return GLASS_SCANCODE_L;
        case SDL_SCANCODE_M           : return GLASS_SCANCODE_M;
        case SDL_SCANCODE_N           : return GLASS_SCANCODE_N;
        case SDL_SCANCODE_O           : return GLASS_SCANCODE_O;
        case SDL_SCANCODE_P           : return GLASS_SCANCODE_P;
        case SDL_SCANCODE_Q           : return GLASS_SCANCODE_Q;
        case SDL_SCANCODE_R           : return GLASS_SCANCODE_R;
        case SDL_SCANCODE_S           : return GLASS_SCANCODE_S;
        case SDL_SCANCODE_T           : return GLASS_SCANCODE_T;
        case SDL_SCANCODE_U           : return GLASS_SCANCODE_U;
        case SDL_SCANCODE_V           : return GLASS_SCANCODE_V;
        case SDL_SCANCODE_W           : return GLASS_SCANCODE_W;
        case SDL_SCANCODE_X           : return GLASS_SCANCODE_X;
        case SDL_SCANCODE_Y           : return GLASS_SCANCODE_Y;
        case SDL_SCANCODE_Z           : return GLASS_SCANCODE_Z;
        case SDL_SCANCODE_KP_0        : return GLASS_SCANCODE_NUM0;
        case SDL_SCANCODE_KP_1        : return GLASS_SCANCODE_NUM1;
        case SDL_SCANCODE_KP_2        : return GLASS_SCANCODE_NUM2;
        case SDL_SCANCODE_KP_3        : return GLASS_SCANCODE_NUM3;
        case SDL_SCANCODE_KP_4        : return GLASS_SCANCODE_NUM4;
        case SDL_SCANCODE_KP_5        : return GLASS_SCANCODE_NUM5;
        case SDL_SCANCODE_KP_6        : return GLASS_SCANCODE_NUM6;
        case SDL_SCANCODE_KP_7        : return GLASS_SCANCODE_NUM7;
        case SDL_SCANCODE_KP_8        : return GLASS_SCANCODE_NUM8;
        case SDL_SCANCODE_KP_9        : return GLASS_SCANCODE_NUM9;
        case SDL_SCANCODE_KP_MULTIPLY : return GLASS_SCANCODE_NUM_MUL;
        case SDL_SCANCODE_KP_PLUS     : return GLASS_SCANCODE_NUM_ADD;
        case SDL_SCANCODE_KP_DIVIDE   : return GLASS_SCANCODE_NUM_DIV;
        case SDL_SCANCODE_KP_MINUS    : return GLASS_SCANCODE_NUM_SUB;
        case SDL_SCANCODE_KP_DECIMAL  : return GLASS_SCANCODE_NUM_DECIMAL;
        case SDL_SCANCODE_KP_ENTER    : return GLASS_SCANCODE_NUM_RETURN;
        case SDL_SCANCODE_F1          : return GLASS_SCANCODE_F1;
        case SDL_SCANCODE_F2          : return GLASS_SCANCODE_F2;
        case SDL_SCANCODE_F3          : return GLASS_SCANCODE_F3;
        case SDL_SCANCODE_F4          : return GLASS_SCANCODE_F4;
        case SDL_SCANCODE_F5          : return GLASS_SCANCODE_F5;
        case SDL_SCANCODE_F6          : return GLASS_SCANCODE_F6;
        case SDL_SCANCODE_F7          : return GLASS_SCANCODE_F7;
        case SDL_SCANCODE_F8          : return GLASS_SCANCODE_F8;
        case SDL_SCANCODE_F9          : return GLASS_SCANCODE_F9;
        case SDL_SCANCODE_F10         : return GLASS_SCANCODE_F10;
        case SDL_SCANCODE_F11         : return GLASS_SCANCODE_F11;
        case SDL_SCANCODE_F12         : return GLASS_SCANCODE_F12;
        case SDL_SCANCODE_NUMLOCKCLEAR: return GLASS_SCANCODE_NUMLOCK;
        case SDL_SCANCODE_SCROLLLOCK  : return GLASS_SCANCODE_SL;
    }

    return GLASS_SCANCODE_UNASSIGNED;
}

void get_all_available_mices(InputDevice** mices, s32* m_count) {
    auto m_mices = SDL_GetMice(m_count);

    *mices = Calloc(InputDevice, *m_count);

    for (s32 i = 0; i < *m_count; i++) {
        InputDevice device{};

        device.type     = MOUSE;
        device.mouse_id = m_mices[i];

        *mices[i] = device;
    }
}