#include <vulkan/vulkan.h>
#include <cstdio>
#include "glass.h"

typedef struct Surface {
    VkSurfaceKHR surface;
} Surface;

extern GlassErrorCode glass_create_surface(Window* window, Surface* surface) {
    return GLASS_OK;
}

#if defined(WIN32)
#define INSTANCE_EXTENSIONS_COUNT 2
const char* INSTANCE_EXTENSIONS[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
};

GlassErrorCode glass_create_vulkan_surface(Window* window, VkInstance vk_instance, VkSurfaceKHR* surface) {
    VkWin32SurfaceCreateInfoKHR surface_info = {
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext     = NULL,
        .hinstance = glass_win32_get_instance(window),
        .hwnd      = glass_win32_get_window_handle(window),
    };

    VkResult result = vkCreateWin32SurfaceKHR(vk_instance,
                                     &surface_info,
                                     NULL,
                                     surface);
    
    if (result != VK_SUCCESS) {
        printf("Cannot create vulkan surface. %d\n", result);
        return GLASS_INTERNAL_ERROR;
    }

    return GLASS_OK;
}

const char** glass_get_vulkan_instance_extensions(u32* count) {
    *count = INSTANCE_EXTENSIONS_COUNT;
    return INSTANCE_EXTENSIONS;
}
#endif