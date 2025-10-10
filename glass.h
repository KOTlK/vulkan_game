#pragma once

#include "basic.h"

enum ScreenMode {
    FULLSCREEN,
    WINDOWED,
    FULLSCREEN_WINDOW,
};

struct Window;
struct Surface;

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

// window management
extern GlassErrorCode glass_create_window(u32 x, u32 y, u32 width, u32 height, const char* name, Window** window);
extern void           glass_destroy_window(Window* window);

extern void           glass_exit();
extern bool           glass_exit_required();
extern void           glass_main_loop();

extern ScreenParams   glass_get_current_screen_params(Window* window);
extern void           glass_set_screen_params(Window* window, ScreenParams params);
extern void           glass_set_window_size(Window* window, u32 width, u32 height);
extern u32            glass_get_available_fullscreen_params_count();
extern void           glass_get_all_available_fullscreen_params(ScreenParams** buffer); // get list of all available fullscreen parameters for currently selected display.

extern double glass_get_time();

extern void glass_set_window_title(Window* window, char* title);

extern GlassErrorCode glass_render();
extern GlassErrorCode glass_on_resize(u32 width, u32 height);

// rendering
extern GlassErrorCode glass_create_surface(Window* window, Surface* surface);

#if defined(WIN32)
#include "windows.h"
extern HWND      glass_win32_get_window_handle(Window* window);
extern HINSTANCE glass_win32_get_instance(Window* window);
#endif

#if defined(Vulkan)
#include <vulkan/vulkan.h>
extern GlassErrorCode glass_create_vulkan_surface(Window* window, VkInstance vk_instance, VkSurfaceKHR* surface);
extern const char**   glass_get_vulkan_instance_extensions(u32* count);

#endif