// #define DVK_USE_PLATFORM_WIN32_KHR
// #define Vulkan

// #include <vulkan/vulkan.h>
#include "glass.h"
#include <cstdio>
#include "basic.h"
#include <cstring>
#define GAME_MATH_IMPLEMENTATION
#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4.h"
#include "geometry.h"
#include <time.h>
#include <stdlib.h>
#include "assert.h"
#include "hash_table.h"
#include "render.h"
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include "debug.h"

#define WIDTH  1280
#define HEIGHT 720

typedef struct Camera2D {
    Vector2 position;
    Vector2 size;
    float   left;
    float   right;
    float   top;
    float   bottom;
} Camera2D;

typedef struct Vertex {
    float position[2];
    u8    color[4];
} Vertex;

typedef struct PerFrameData {
    Matrix4 view;
    Matrix4 proj;
    float   time;
    float   dt;
} PerFrameData;

typedef struct Transform2d {
    Vector2 position;
    Vector2 scale;
    float   rotation;
} Transform2d;

typedef struct Matrices {
    Matrix4 model;
    Matrix4 mvp;
} Matrices;

Window*          WINDOW{};
// Transform2d*   TRANSFORMS;
// Matrices*      MATRICES;
// u32            TRANSFORMS_COUNT  = 0;
// u32            TRANSFORMS_LENGTH = 0;

Matrix4 VIEW;
Matrix4 PROJECTION;

Camera2D CAMERA = {};

double TIME_DOUBLE = 0.0l;
float  TIME        = 0.0f;
float  DT          = 0.0f;

void print_instance_extensions();

static inline float frand01() {
    return (float)rand() / RAND_MAX;
}

static inline float frand(float min, float max) {
    float t = frand01();
    return (1.0f - t) * min + t * max;
}

int main(int argc, char** argv) {
    // srand(time(NULL));

    // Vertex2D shape_vertices[] = {
    //     {{{   0.5f,  -0.5f }}, { 255, 0,   0,   255 }},
    //     {{{   0.5f,   0.5f }}, { 0,   255, 0,   255 }},
    //     {{{  -0.5f,   0.5f }}, { 0,   0,   255, 255 }},
    //     {{{  -0.5f,  -0.5f }}, { 255, 255, 255, 255 }},
    // };

    // u16 shape_indices[] = {
    //     0, 1, 2, 0, 2, 3
    // };

    // Shape2D shape;

    // shape2d_make(shape_vertices, shape_indices, 4, 6, &shape);

    // for (u32 i = 0; i < shape.vertex_count; i++) {
    //     Vertex2D v = shape.vertices[i];
    //     printf("%f, %f, %d, %d, %d, %d\n", v.position.x, v.position.y, v.color.r, v.color.g, v.color.b, v.color.a);
    // }

    // for (u32 i = 0; i < shape.index_count; i++) {
    //     printf("%d\n", shape.indices[i]);
    // }

    const char* name         = "Hello";

    GlassErrorCode err;

    WINDOW = glass_create_window(100, 100, WIDTH, HEIGHT, name, &err);

    if (err != GLASS_OK) {
        printf("Cannot create window. %d\n", err);
        return 1;
    }

    RenderError render_err = render_init(WINDOW);

    if (render_err != RENDER_OK) {
        printf("Render init error. %d.\n", render_err);
        return 1;
    }


    // SDL_Window *window = SDL_CreateWindow(
    //     "Hello sdl",
    //     800, 600,
    //     SDL_WINDOW_OPENGL | 
    //     SDL_WINDOW_RESIZABLE |
    //     SDL_WINDOW_INPUT_FOCUS
    // );
    
    // SDL_Init(SDL_INIT_VIDEO);

    // SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // SDL_GLContext context = SDL_GL_CreateContext(window);

    // int version = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    // if (version == 0) {
    //     Errf("Cannot load glad.");
    //     return 0;
    // }

    // Logf("Glad version: %i.", version);

    while (true) {
        free_temp_allocator();
        if (glass_is_button_pressed(WINDOW, GLASS_SCANCODE_ESCAPE)) {
            glass_exit();
            break;
        }

        if (glass_exit_required())
            break;

        glass_main_loop();
        glass_sleep(0.016l);
    }

    // int exit = 0;
    // while(!exit) {
    //     SDL_Event event;
    //     while (SDL_PollEvent(&event)) {
    //         switch(event.type) {
    //             case SDL_EVENT_QUIT:
    //                 exit = 1;
    //                 break;
    //             case SDL_EVENT_KEY_DOWN:
    //                 if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
    //                     exit = 1;
    //                 }
    //                 break;
    //             case SDL_EVENT_WINDOW_RESIZED: {
    //                 Logf("Resized. %i, %i.", event.window.data1, event.window.data2);
    //             } break;
    //             default:
    //                 break;
    //         }
    //     }

    //     glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    //     glClear(GL_COLOR_BUFFER_BIT);

    //     SDL_GL_SwapWindow(window);
    //     SDL_Delay(1);
    // }

    // SDL_GL_DestroyContext(context);
    // SDL_DestroyWindow(window);
    // SDL_Quit();

    render_destroy();
    glass_destroy_window(WINDOW);

    // double last_time = glass_get_time();

    // const double target_fps = 1.0l / 60.0l;

    // while (true) {
    //     free_temp_allocator();
    //     if (glass_is_button_pressed(GLASS_SCANCODE_ESCAPE)) {
    //         glass_exit();
    //         break;
    //     }

    //     if (glass_exit_required())
    //         break;

    //     glass_main_loop();
        
    //     double current_time = glass_get_time();
    //     double actual_dt    = current_time - last_time;
    //            TIME_DOUBLE  += actual_dt;
    //     last_time = current_time;
    //     char buf[128];
        
    //     double sleep_time = target_fps - actual_dt;

    //     if (actual_dt > 0.0l) {
    //         glass_sleep(sleep_time);
    //         last_time = glass_get_time();
    //         actual_dt = target_fps;
    //     }

    //     int   fps  = (int)(1.0l / actual_dt);
    //     float dt   = (float)actual_dt;
    //           DT   = dt;
    //           TIME = float(TIME_DOUBLE);
    //     sprintf(buf, "fps: %i, dt:%f, time:%f", fps, dt, TIME);
    //     glass_set_window_title(WINDOW, buf);
    // }

    // printf("Exit begin.\n");

    // render_destroy();
    // glass_destroy_window(WINDOW);
    return 0;
}

GlassErrorCode glass_render() {
    RenderError err = render_test();

    if (err != RENDER_OK) {
        printf("Cannot render. Render error: %d.\n", err);
        return GLASS_INTERNAL_ERROR;
    }

    return GLASS_OK;
}

// GlassErrorCode glass_on_resize(u32 width, u32 height) {
//     return GLASS_OK;
// }

// GlassErrorCode glass_on_move(u32 x, u32 y) {
//     return GLASS_OK;
// }