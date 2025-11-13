#include "windows.h"
#include <cstdio>
#include "glass.h"

#define START_WINDOWS_LENGTH 2
#define WINDOWS_LENGTH_STEP  4
#define MAX_BUTTONS          16
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

typedef struct KeyState {
    bool hold;
} KeyState;

static Window* OPEN_WINDOWS   = NULL;
static u32     WINDOWS_COUNT  = 0;
static u32     WINDOWS_LENGTH = 0;

static KeyState* KEYS;

LRESULT CALLBACK            WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static inline int           get_cmd_show();
static inline GlassScancode vkey_to_scancode(int vkey);

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

    if (KEYS == NULL) {
        KEYS = Calloc(KeyState, SCANCODE_COUNT);
        memset(KEYS, 0, sizeof(KeyState) * SCANCODE_COUNT);
    }

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

void glass_get_window_size(Window* window, u32* width, u32* height) {
    *width  = window->width;
    *height = window->height;
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

// input
bool glass_is_button_pressed(GlassScancode scancode) {
    return KEYS[scancode].hold;
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
        case WM_DESTROY: {
            PostQuitMessage(0);
            glass_exit();
        } return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            // FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
        } return 0;
        case WM_SIZE: {
            int width  = LOWORD(lParam);
            int height = HIWORD(lParam);

            printf("Resize: %i, %i\n", width, height);
            glass_on_resize(width, height);
        } return 0;
        case WM_KEYDOWN: {
            KEYS[vkey_to_scancode(wParam)].hold = true;
        } return 0;
        case WM_KEYUP: {
            KEYS[vkey_to_scancode(wParam)].hold = false;
        } return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static inline GlassScancode vkey_to_scancode(int vkey) {
    switch (vkey) {
        // Mouse buttons
        case VK_LBUTTON : return GLASS_SCANCODE_MOUSE0;
        case VK_RBUTTON : return GLASS_SCANCODE_MOUSE1;
        case VK_MBUTTON : return GLASS_SCANCODE_MOUSE2;
        case VK_XBUTTON1: return GLASS_SCANCODE_MOUSE3;
        case VK_XBUTTON2: return GLASS_SCANCODE_MOUSE4;
        
        // Main keys
        case VK_TAB     : return GLASS_SCANCODE_TAB;
        case VK_BACK    : return GLASS_SCANCODE_BACKSPACE;
        case VK_RETURN  : return GLASS_SCANCODE_RETURN;
        case VK_LSHIFT  : return GLASS_SCANCODE_LEFT_SHIFT;
        case VK_RSHIFT  : return GLASS_SCANCODE_RIGHT_SHIFT;
        case VK_LCONTROL: return GLASS_SCANCODE_LEFT_CONTROL;
        case VK_RCONTROL: return GLASS_SCANCODE_RIGHT_CONTROL;
        case VK_LMENU   : return GLASS_SCANCODE_LEFT_ALT;
        case VK_RMENU   : return GLASS_SCANCODE_RIGHT_ALT;
        case VK_PAUSE   : return GLASS_SCANCODE_PAUSE;
        case VK_CAPITAL : return GLASS_SCANCODE_CAPS;
        case VK_ESCAPE  : return GLASS_SCANCODE_ESCAPE;
        case VK_SPACE   : return GLASS_SCANCODE_SPACE;
        case VK_PRIOR   : return GLASS_SCANCODE_PAGEUP;
        case VK_NEXT    : return GLASS_SCANCODE_PAGEDOWN;
        case VK_END     : return GLASS_SCANCODE_END;
        case VK_HOME    : return GLASS_SCANCODE_HOME;
        case VK_LEFT    : return GLASS_SCANCODE_LEFT;
        case VK_UP      : return GLASS_SCANCODE_UP;
        case VK_RIGHT   : return GLASS_SCANCODE_RIGHT;
        case VK_DOWN    : return GLASS_SCANCODE_DOWN;
        case VK_SNAPSHOT: return GLASS_SCANCODE_PRINTSCREEN;
        case VK_INSERT  : return GLASS_SCANCODE_INSERT;
        case VK_DELETE  : return GLASS_SCANCODE_DELETE;
        
        // Alpha keys
        case '0': return GLASS_SCANCODE_ALPHA0;
        case '1': return GLASS_SCANCODE_ALPHA1;
        case '2': return GLASS_SCANCODE_ALPHA2;
        case '3': return GLASS_SCANCODE_ALPHA3;
        case '4': return GLASS_SCANCODE_ALPHA4;
        case '5': return GLASS_SCANCODE_ALPHA5;
        case '6': return GLASS_SCANCODE_ALPHA6;
        case '7': return GLASS_SCANCODE_ALPHA7;
        case '8': return GLASS_SCANCODE_ALPHA8;
        case '9': return GLASS_SCANCODE_ALPHA9;
        case 'A': return GLASS_SCANCODE_A;
        case 'B': return GLASS_SCANCODE_B;
        case 'C': return GLASS_SCANCODE_C;
        case 'D': return GLASS_SCANCODE_D;
        case 'E': return GLASS_SCANCODE_E;
        case 'F': return GLASS_SCANCODE_F;
        case 'G': return GLASS_SCANCODE_G;
        case 'H': return GLASS_SCANCODE_H;
        case 'I': return GLASS_SCANCODE_I;
        case 'J': return GLASS_SCANCODE_J;
        case 'K': return GLASS_SCANCODE_K;
        case 'L': return GLASS_SCANCODE_L;
        case 'M': return GLASS_SCANCODE_M;
        case 'N': return GLASS_SCANCODE_N;
        case 'O': return GLASS_SCANCODE_O;
        case 'P': return GLASS_SCANCODE_P;
        case 'Q': return GLASS_SCANCODE_Q;
        case 'R': return GLASS_SCANCODE_R;
        case 'S': return GLASS_SCANCODE_S;
        case 'T': return GLASS_SCANCODE_T;
        case 'U': return GLASS_SCANCODE_U;
        case 'V': return GLASS_SCANCODE_V;
        case 'W': return GLASS_SCANCODE_W;
        case 'X': return GLASS_SCANCODE_X;
        case 'Y': return GLASS_SCANCODE_Y;
        case 'Z': return GLASS_SCANCODE_Z;
        
        // Windows keys
        case VK_LWIN: return GLASS_SCANCODE_LEFT_WIN;
        case VK_RWIN: return GLASS_SCANCODE_RIGHT_WIN;
        
        // Numpad
        case VK_NUMPAD0 : return GLASS_SCANCODE_NUM0;
        case VK_NUMPAD1 : return GLASS_SCANCODE_NUM1;
        case VK_NUMPAD2 : return GLASS_SCANCODE_NUM2;
        case VK_NUMPAD3 : return GLASS_SCANCODE_NUM3;
        case VK_NUMPAD4 : return GLASS_SCANCODE_NUM4;
        case VK_NUMPAD5 : return GLASS_SCANCODE_NUM5;
        case VK_NUMPAD6 : return GLASS_SCANCODE_NUM6;
        case VK_NUMPAD7 : return GLASS_SCANCODE_NUM7;
        case VK_NUMPAD8 : return GLASS_SCANCODE_NUM8;
        case VK_NUMPAD9 : return GLASS_SCANCODE_NUM9;
        case VK_MULTIPLY: return GLASS_SCANCODE_NUM_MUL;
        case VK_ADD     : return GLASS_SCANCODE_NUM_ADD;
        case VK_DIVIDE  : return GLASS_SCANCODE_NUM_DIV;
        case VK_SUBTRACT: return GLASS_SCANCODE_NUM_SUB;
        case VK_DECIMAL : return GLASS_SCANCODE_NUM_DECIMAL;
        
        // Function keys
        case VK_F1 : return GLASS_SCANCODE_F1;
        case VK_F2 : return GLASS_SCANCODE_F2;
        case VK_F3 : return GLASS_SCANCODE_F3;
        case VK_F4 : return GLASS_SCANCODE_F4;
        case VK_F5 : return GLASS_SCANCODE_F5;
        case VK_F6 : return GLASS_SCANCODE_F6;
        case VK_F7 : return GLASS_SCANCODE_F7;
        case VK_F8 : return GLASS_SCANCODE_F8;
        case VK_F9 : return GLASS_SCANCODE_F9;
        case VK_F10: return GLASS_SCANCODE_F10;
        case VK_F11: return GLASS_SCANCODE_F11;
        case VK_F12: return GLASS_SCANCODE_F12;
        
        // Other
        case VK_NUMLOCK: return GLASS_SCANCODE_NUMLOCK;
        case VK_SCROLL : return GLASS_SCANCODE_SL;
        
        default: return GLASS_SCANCODE_UNASSIGNED;
    }
}