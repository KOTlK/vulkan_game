#include "glass.h"
#include <cstdio>
#include "basic.h"
#include <cstring>
#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4.h"
#include "geometry.h"
#include <time.h>
#include <stdlib.h>
#include "assert.h"
#include "hash_table.h"
#include "render.h"
#include "debug.h"
#include "game_context.h"
#include "array.h"
#include "mathematics.h"
#include "vector_utils.h"
#include "rlist.h"

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

Context Game_Context{};
// Window*          Wnd{};
// Transform2d*   TRANSFORMS;
// Matrices*      MATRICES;
// u32            TRANSFORMS_COUNT  = 0;
// u32            TRANSFORMS_LENGTH = 0;

Matrix4 VIEW;
Matrix4 PROJECTION;

Camera2D CAMERA = {};

// double TIME_DOUBLE = 0.0l;
// float  TIME        = 0.0f;
// float  DT          = 0.0f;

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

    // Vertex shape_vertices[] = {
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
    //     Vertex v = shape.vertices[i];
    //     printf("%f, %f, %d, %d, %d, %d\n", v.position.x, v.position.y, v.color.r, v.color.g, v.color.b, v.color.a);
    // }

    // for (u32 i = 0; i < shape.index_count; i++) {
    //     printf("%d\n", shape.indices[i]);
    // }

    const char* name = "Hello";

    GlassErrorCode err = GLASS_OK;

    Game_Context.wnd = glass_create_window(400, 100, WIDTH, HEIGHT, name, &err);

    if (err != GLASS_OK) {
        Errf("Cannot create window. %d", err);
        return 1;
    }

    Log("Window created.");

    RenderError render_err = render_init(&Game_Context);

    if (render_err != RENDER_OK) {
        Errf("Render init error. %d.", render_err);
        return 1;
    }

    Log("Render initialized.");

    u64 last_time = glass_query_performance_counter();
    u64 current_time = 0;

    u64 max_fps           = 1000;
    u64 target_fps        = 75;
    u64 target_frame_time = 1000 / target_fps;

#if MEMORY_DEBUG
    s64 persistent_memory_this_frame = 0;
#endif
    while (true) {

#if MEMORY_DEBUG
        Arena* arena = static_cast<Arena*>(get_temp_allocator());
        Logf("Temp memory allocated this frame: %llu B, %lf KB, %lf MB \n"
             "Total temp memory capacity: %llu B, %lf KB, %lf MB\n"
             "Buckets allocated: %llu", arena->allocated,
                                        arena->allocated / 1024.0,
                                        arena->allocated / 1024.0 / 1024.0,
                                        arena->total_capacity,
                                        arena->total_capacity / 1024.0,
                                        arena->total_capacity / 1024.0 / 1024.0,
                                        arena->buckets_count);

        AllocatorPersistent* persistent = static_cast<AllocatorPersistent*>(Allocator_Persistent);
        s64 mem_diff = (s64)persistent->allocated - persistent_memory_this_frame;
        persistent_memory_this_frame = (s64)persistent->allocated;
        Logf("Persistent memory allocated totally: %llu B, %lf KB, %lf MB \n"
             "Persistent memory allocated this frame: %lli B, %lf KB, %lf MB \n",
                                        persistent->allocated,
                                        persistent->allocated / 1024.0,
                                        persistent->allocated / 1024.0 / 1024.0,
                                        mem_diff,
                                        mem_diff / 1024.0,
                                        mem_diff / 1024.0 / 1024.0);
#endif
        free_temp_allocator();
        if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_ESCAPE)) {
            glass_exit();
            break;
        }

        if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_UP)) {
            target_fps += 1;
            target_fps = clamp(target_fps, 1ull, max_fps);
            target_frame_time = 1000 / target_fps;
        } else if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_DOWN)) {
            target_fps -= 1;
            target_fps = clamp(target_fps, 1ull, max_fps);
            target_frame_time = 1000 / target_fps;
        }

        if (glass_exit_required())
            break;

        glass_main_loop();

        current_time = glass_query_performance_counter();

        u64 dt_int = (current_time - last_time) * 1000 / glass_query_performance_frequency();
        double dt = (double)(current_time - last_time) / glass_query_performance_frequency();

        last_time = current_time;

        u64 sleep_time = target_frame_time - dt_int;

        sleep_time = clamp(sleep_time, 0llu, target_frame_time);

        if (sleep_time > 0) {
            glass_sleep(sleep_time);
            current_time = glass_query_performance_counter();

            dt = (double)(current_time - last_time) / glass_query_performance_frequency();
            last_time = current_time;
        }

        Game_Context.time.dt_double    = dt;
        Game_Context.time.dt           = (float)dt;
        Game_Context.time.time_double += dt;
        Game_Context.time.time         = (float)Game_Context.time.time_double;

        u64 fps = (u64)(1.0l / dt);

        // Logf("Delta Time: %f", dt);
        // Logf("Target frame time: %llu", target_frame_time);
        // Logf("Sleep time: %llu", sleep_time);
        // Logf("Dt int: %llu", dt_int);

        char buf[1024];

        sprintf(buf, "fps: %llu, dt:%f, time:%f\n", fps, Game_Context.time.dt, Game_Context.time.time);
        glass_set_window_title(Game_Context.wnd, buf);

        // glass_sleep(16);
    }

    render_destroy();
    glass_destroy_window(Game_Context.wnd);

    return 0;
}

GlassErrorCode glass_render(Window* window) {
    RenderError err = render_test();


    if (err != RENDER_OK) {
        Errf("Cannot render. Render error: %d.", err);
        return GLASS_INTERNAL_ERROR;
    }

    return GLASS_OK;
}

GlassErrorCode glass_on_resize(u32 width, u32 height) {
    Logf("Resized. %i, %i.", width, height);
    return GLASS_OK;
}

GlassErrorCode glass_on_move(u32 x, u32 y) {
    Logf("Moved. %i, %i.", x, y);
    return GLASS_OK;
}