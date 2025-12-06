#pragma once

// #include "basic.h"
#include "types.h"

enum ScreenMode {
    FULLSCREEN,
    WINDOWED,
    FULLSCREEN_WINDOW,
};

struct Window;
struct Surface;
struct Application;

struct ScreenParams {
    u32 width;
    u32 height;
    u32 x;
    u32 y;
    u32 refresh_rate;
    ScreenMode screen_mode;
};

enum GlassErrorCode {
    GLASS_OK                = 0,
    GLASS_NOT_ENOUGH_MEMORY = 1,
    GLASS_WINAPI_ERROR      = 2,
    GLASS_RENDER_ERROR      = 3,
    GLASS_INTERNAL_ERROR    = 4,
};

#define SCANCODE_COUNT 98
enum GlassScancode {
    GLASS_SCANCODE_UNASSIGNED    = 0,
    GLASS_SCANCODE_MOUSE0        = 1,    // lmb
    GLASS_SCANCODE_MOUSE1        = 2,    // rmb
    GLASS_SCANCODE_MOUSE2        = 3,    // mmb
    GLASS_SCANCODE_MOUSE3        = 4,
    GLASS_SCANCODE_MOUSE4        = 5,
    GLASS_SCANCODE_TAB           = 6,
    GLASS_SCANCODE_BACKSPACE     = 7,
    GLASS_SCANCODE_RETURN        = 8,
    GLASS_SCANCODE_LEFT_SHIFT    = 9,
    GLASS_SCANCODE_RIGHT_SHIFT   = 10,
    GLASS_SCANCODE_LEFT_CONTROL  = 11,
    GLASS_SCANCODE_RIGHT_CONTROL = 12,
    GLASS_SCANCODE_LEFT_ALT      = 13,
    GLASS_SCANCODE_RIGHT_ALT     = 14,
    GLASS_SCANCODE_PAUSE         = 15,
    GLASS_SCANCODE_CAPS          = 16,
    GLASS_SCANCODE_ESCAPE        = 17,
    GLASS_SCANCODE_SPACE         = 18,
    GLASS_SCANCODE_PAGEUP        = 19,
    GLASS_SCANCODE_PAGEDOWN      = 20,
    GLASS_SCANCODE_END           = 21,
    GLASS_SCANCODE_HOME          = 22,
    GLASS_SCANCODE_LEFT          = 23,
    GLASS_SCANCODE_UP            = 24,
    GLASS_SCANCODE_RIGHT         = 25,
    GLASS_SCANCODE_DOWN          = 26,
    GLASS_SCANCODE_PRINTSCREEN   = 27,
    GLASS_SCANCODE_INSERT        = 28,
    GLASS_SCANCODE_DELETE        = 29,
    GLASS_SCANCODE_ALPHA0        = 30,
    GLASS_SCANCODE_ALPHA1        = 31,
    GLASS_SCANCODE_ALPHA2        = 32,
    GLASS_SCANCODE_ALPHA3        = 33,
    GLASS_SCANCODE_ALPHA4        = 34,
    GLASS_SCANCODE_ALPHA5        = 35,
    GLASS_SCANCODE_ALPHA6        = 36,
    GLASS_SCANCODE_ALPHA7        = 37,
    GLASS_SCANCODE_ALPHA8        = 38,
    GLASS_SCANCODE_ALPHA9        = 39,
    GLASS_SCANCODE_A             = 40,
    GLASS_SCANCODE_B             = 41,
    GLASS_SCANCODE_C             = 42,
    GLASS_SCANCODE_D             = 43,
    GLASS_SCANCODE_E             = 44,
    GLASS_SCANCODE_F             = 45,
    GLASS_SCANCODE_G             = 46,
    GLASS_SCANCODE_H             = 47,
    GLASS_SCANCODE_I             = 48,
    GLASS_SCANCODE_J             = 49,
    GLASS_SCANCODE_K             = 50,
    GLASS_SCANCODE_L             = 51,
    GLASS_SCANCODE_M             = 52,
    GLASS_SCANCODE_N             = 53,
    GLASS_SCANCODE_O             = 54,
    GLASS_SCANCODE_P             = 55,
    GLASS_SCANCODE_Q             = 56,
    GLASS_SCANCODE_R             = 57,
    GLASS_SCANCODE_S             = 58,
    GLASS_SCANCODE_T             = 59,
    GLASS_SCANCODE_U             = 60,
    GLASS_SCANCODE_V             = 61,
    GLASS_SCANCODE_W             = 62,
    GLASS_SCANCODE_X             = 63,
    GLASS_SCANCODE_Y             = 64,
    GLASS_SCANCODE_Z             = 65,
    #if defined(WIN32)
    GLASS_SCANCODE_LEFT_WIN  = 66,
    GLASS_SCANCODE_RIGHT_WIN = 67,
    #endif
    GLASS_SCANCODE_NUM0        = 68,
    GLASS_SCANCODE_NUM1        = 69,
    GLASS_SCANCODE_NUM2        = 70,
    GLASS_SCANCODE_NUM3        = 71,
    GLASS_SCANCODE_NUM4        = 72,
    GLASS_SCANCODE_NUM5        = 73,
    GLASS_SCANCODE_NUM6        = 74,
    GLASS_SCANCODE_NUM7        = 75,
    GLASS_SCANCODE_NUM8        = 76,
    GLASS_SCANCODE_NUM9        = 77,
    GLASS_SCANCODE_NUM_MUL     = 78,
    GLASS_SCANCODE_NUM_ADD     = 79,
    GLASS_SCANCODE_NUM_DIV     = 80,
    GLASS_SCANCODE_NUM_SUB     = 81,
    GLASS_SCANCODE_NUM_DECIMAL = 82,
    GLASS_SCANCODE_NUM_RETURN  = 83,
    GLASS_SCANCODE_F1          = 84,
    GLASS_SCANCODE_F2          = 85,
    GLASS_SCANCODE_F3          = 86,
    GLASS_SCANCODE_F4          = 87,
    GLASS_SCANCODE_F5          = 88,
    GLASS_SCANCODE_F6          = 89,
    GLASS_SCANCODE_F7          = 90,
    GLASS_SCANCODE_F8          = 91,
    GLASS_SCANCODE_F9          = 92,
    GLASS_SCANCODE_F10         = 93,
    GLASS_SCANCODE_F11         = 94,
    GLASS_SCANCODE_F12         = 95,
    GLASS_SCANCODE_NUMLOCK     = 96,
    GLASS_SCANCODE_SL          = 97,   // SCROLL LOCK
};

// window management
extern Window*        glass_create_window(u32 x, u32 y, u32 width, u32 height, const char* name, GlassErrorCode* err);
extern void           glass_destroy_window(Window* window);

extern void           glass_exit();
extern bool           glass_exit_required();
extern void           glass_main_loop();

extern void           glass_get_window_size(Window* window, u32* width, u32* height);
extern ScreenParams   glass_get_current_screen_params(Window* window);
extern void           glass_set_screen_params(Window* window, ScreenParams params);
extern void           glass_set_window_size(Window* window, u32 width, u32 height);
extern u32            glass_get_available_fullscreen_params_count();
extern void           glass_get_all_available_fullscreen_params(ScreenParams** buffer); // get list of all available fullscreen parameters for currently selected display.

extern double glass_get_time();
extern void   glass_sleep(double time);

extern void glass_set_window_title(Window* window, char* title);

extern GlassErrorCode glass_render();
extern GlassErrorCode glass_on_resize(u32 width, u32 height);
extern GlassErrorCode glass_on_move(u32 x, u32 y);
extern GlassErrorCode glass_swap_buffers(Window* window);
extern void*          glass_get_proc_addr(const char* name);

#if defined(WIN32)
#include "windows.h"
extern HWND      glass_win32_get_window_handle(Window* window);
extern HINSTANCE glass_win32_get_instance(Window* window);
#endif

extern Surface* glass_create_surface();

// rendering
// extern GlassErrorCode glass_create_surface(Window* window, Surface* surface);
// extern GlassErrorCode glass_render_init(Window* window, Application* app);

#if defined(Vulkan)
#include <vulkan/vulkan.h>
extern GlassErrorCode glass_create_vulkan_surface(Window* window, VkInstance vk_instance, VkSurfaceKHR* surface);
extern const char**   glass_get_vulkan_instance_extensions(u32* count);
#endif

// input
extern bool glass_is_button_pressed(Window* win, GlassScancode scancode);