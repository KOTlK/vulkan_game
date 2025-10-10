#include "windows.h"
#include <cstdio>
#include "glass.h"

#define START_WINDOWS_LENGTH 2
#define WINDOWS_LENGTH_STEP  4
// static HINSTANCE INSTANCE;
// static HWND      WINDOW;
static bool      SHOULD_QUIT = false;

typedef struct Window {
    HINSTANCE instance;
    HWND      window;
    u32       width;
    u32       height;
    u32       x;
    u32       y;
    bool      should_close;
} Window;

static Window* OPEN_WINDOWS   = NULL;
static u32     WINDOWS_COUNT  = 0;
static u32     WINDOWS_LENGTH = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static inline int get_cmd_show();

GlassErrorCode glass_create_window(u32 x, u32 y, u32 width, u32 height, const char* name, Window** window) {
    WNDCLASSA window_class{};
    HINSTANCE instance = GetModuleHandle(null);

    window_class.lpfnWndProc   = WindowProc;
    window_class.hInstance     = instance;
    window_class.lpszClassName = name;

    RegisterClassA(&window_class);

    HWND hwnd = CreateWindowA(
        name,
        name,
        WS_OVERLAPPEDWINDOW,

        // Size and position
        x, y, width, height,

        NULL,       // Parent window
        NULL,       // Menu
        instance,   // INSTANCE handle
        NULL        // Additional application data
    );

    if (hwnd == null) {
        glass_exit();
        return GLASS_WINAPI_ERROR;
    }

    ShowWindow(hwnd, get_cmd_show());

    Window wnd {
        .instance     = instance,
        .window       = hwnd,
        .width        = width,
        .height       = height,
        .x            = x,
        .y            = y,
        .should_close = false,
    };

    if (WINDOWS_COUNT == 0 && OPEN_WINDOWS == NULL) {
        OPEN_WINDOWS   = Calloc(Window, START_WINDOWS_LENGTH);
        if (!OPEN_WINDOWS) {
            return GLASS_NOT_ENOUGH_MEMORY;
        }
        WINDOWS_LENGTH = START_WINDOWS_LENGTH;
    } else if (WINDOWS_COUNT >= WINDOWS_LENGTH) {
        WINDOWS_LENGTH += WINDOWS_LENGTH_STEP;
        OPEN_WINDOWS   = Realloc(Window, OPEN_WINDOWS, WINDOWS_LENGTH * sizeof(Window));
        if (!OPEN_WINDOWS) {
            return GLASS_NOT_ENOUGH_MEMORY;
        }
    }

    OPEN_WINDOWS[WINDOWS_COUNT] = wnd;
    *window = &OPEN_WINDOWS[WINDOWS_COUNT];
    WINDOWS_COUNT++;

    return GLASS_OK;
}

void glass_destroy_window(Window* window) {
    DestroyWindow(window->window);
}

void glass_exit() {
    printf("EXIT\n");
    SHOULD_QUIT = true;
    // exit(0);
}

bool glass_exit_required() {
    return SHOULD_QUIT;
}

void glass_main_loop() {
    // printf("main_loop\n");
    for (u32 i = 0; i < WINDOWS_COUNT; i++) {
        UpdateWindow(OPEN_WINDOWS[i].window);
    }

    MSG msg = {};

    while (PeekMessage(&msg, null, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GlassErrorCode render = glass_render();

    if (render != GLASS_OK) {
        glass_exit();
    }
}

double glass_get_time() {
    LARGE_INTEGER frequency, time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);
    return (double)time.QuadPart / frequency.QuadPart;
}

void glass_set_window_title(Window* window, char* title) {
    SetWindowTextA(window->window, title);
}

HWND glass_win32_get_window_handle(Window* window) {
    return window->window;
}

HINSTANCE glass_win32_get_instance(Window* window) {
    return window->instance;
}


static inline
int
get_cmd_show() {
    STARTUPINFO si;
    si.cb = sizeof(si);
    GetStartupInfo(&si);

    if (si.dwFlags & STARTF_USESHOWWINDOW) {
        return si.wShowWindow;
    }

    return SW_SHOWDEFAULT;
}

LRESULT CALLBACK WindowProc(HWND hwnd, 
                            UINT uMsg,
                            WPARAM wParam, 
                            LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY: 
            PostQuitMessage(0);
            glass_exit();
        return 0;
        case WM_PAINT: 
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            // FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
        return 0;
        case WM_SIZE: 
            int width  = LOWORD(lParam);
            int height = HIWORD(lParam);

            printf("Resize: %i, %i\n", width, height);
            glass_on_resize(width, height);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}