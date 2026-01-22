#define GAME_MATH_IMPLEMENTATION
#define TEXT_IMPLEMENTATION
#define FILE_IMPLEMENTATION
#define BITMAP_IMPLEMENTATION
#include "glass.h"
#include <cstdio>
#include "basic.h"
#include <cstring>
#include "geometry.h"
#include <time.h>
#include <stdlib.h>
#include "assert.h"
#include "render.h"
#include "debug.h"
#include "game_context.h"
#include "components.h"
#include "component_system.h"
#include "file.h"
#include "context.h"

import list;
import hash_table;
import array;
import queue;
import rlist;
import vector3;
import vector2;
import vector4;
import vector4;
import math;
import bitmap;
import text;

#define WIDTH  1280
#define HEIGHT 720

Game_Context G_Context{};

Matrix4 VIEW;
Matrix4 PROJECTION;

Camera  Cam;

static EntityManager em;
static Material* Active_Material;
static Shape2D   Shape;

static Transform Test_Transform = {
    .position = {{0, 0, 0}},
    .rotation = quaternion_identity,
    .scale    = {{1, 1, 1}},
};

static Vector3 Camera_Rotation = vector3_make(0, radians(0.0f), 0);
static Vector3 Transform_Rotation = vector3_make(0, 0, 0);

static u64 Target_Fps = 75;

static inline float frand01() {
    return (float)rand() / RAND_MAX;
}

static inline float frand(float min, float max) {
    float t = frand01();
    return (1.0f - t) * min + t * max;
}

static inline Vector3 vector3_random(Vector3 min, Vector3 max) {
    return Vector3 {
        .x = frand(min.x, max.x),
        .y = frand(min.y, max.y),
        .z = frand(min.z, max.z),
    };
}

int main(int argc, char** argv) {
    entity_manager_make(&em);

    const char* name = "Hello";

    GlassErrorCode err = GLASS_OK;

    G_Context.wnd = glass_create_window(400, 100, WIDTH, HEIGHT, name, &err);

    if (err != GLASS_OK) {
        Errf("Cannot create window. %d", err);
        return 1;
    }

    Log("Window created.");

    float aspect_ratio = (float)WIDTH / (float)HEIGHT;

    // camera_make_ortho(vector3_make(0,0,0), 0, 10, aspect_ratio, &Cam);
    camera_make_perspective(vector3_make(0, 0, -5.0f), quaternion_euler(Camera_Rotation), aspect_ratio, radians(60.0f), 0.1f, 20000.0f, &Cam);

    RenderError render_err = render_init(&G_Context);

    if (render_err != RENDER_OK) {
        Errf("Render init error. %d.", render_err);
        return 1;
    }

    Log("Render initialized.");

    bool read = false;

    String vert_text;
    String frag_text;

    char buf[512];

    sprintf(buf, "%s%s", glass_get_executable_path(), "shaders/vert.shader");

    read = read_entire_file(buf, Allocator_Temp, &vert_text);

    if (!read) {
        Err("Cannot read vertex shader.");
    }

    sprintf(buf, "%s%s", glass_get_executable_path(), "shaders/frag.shader");

    read = read_entire_file(buf, Allocator_Temp, &frag_text);

    if (!read) {
        Err("Cannot read fragment shader.");
    }

    Shader* shader = shader_make(&vert_text, &frag_text, &render_err);
    Active_Material = material_make(shader);

    if (err) {
        Errf("Cannot create active shader. %d", err);
        return RENDER_INTERNAL_ERROR;
    }

    // Vertex vertices[] = {
    //     {{{  0.0f,   0.5f, 0.0f }}, { 255, 0,   0,   255 }},
    //     {{{ -0.5f,   0.0f, 0.0f }}, { 0,   255, 0,   255 }},
    //     {{{  0.5f,   0.0f, 0.0f }}, { 0,   0,   255, 255 }},
    // };

    // u16 indices[] = {
    //     0, 1, 2,
    // };  
    // Vertex vertices[] = {
    //     {{{   0.5f,  -0.5f, 0.0f }}, { 255, 0,   0,   255 }},
    //     {{{   0.5f,   0.5f, 0.0f }}, { 0,   255, 0,   255 }},
    //     {{{  -0.5f,   0.5f, 0.0f }}, { 0,   0,   255, 255 }},
    //     {{{  -0.5f,  -0.5f, 0.0f }}, { 255, 255, 0, 255 }},
    // };

    // u16 indices[] = {
    //     0, 1, 2,
    //     0, 2, 3
    // };  

    Vertex vertices[] = {
        {{{-0.5f, -0.5f, -0.5f}}, {255, 0, 0, 255}},
        {{{ 0.5f, -0.5f, -0.5f}}, {0, 255, 0, 255}},
        {{{ 0.5f,  0.5f, -0.5f}}, {0, 0, 255, 255}},
        {{{-0.5f,  0.5f, -0.5f}}, {255, 255, 0, 255}},
        {{{-0.5f, -0.5f,  0.5f}}, {255, 0, 0, 255}},
        {{{ 0.5f, -0.5f,  0.5f}}, {0, 255, 0, 255}},
        {{{ 0.5f,  0.5f,  0.5f}}, {0, 0, 255, 255}},
        {{{-0.5f,  0.5f,  0.5f}}, {255, 255, 0, 255}},
    };

    u16 indices[] = {
        0, 1, 3, 3, 1, 2,
        1, 5, 2, 2, 5, 6,
        5, 4, 6, 6, 4, 7,
        4, 0, 7, 7, 0, 3,
        3, 2, 7, 7, 2, 6,
        4, 5, 0, 0, 5, 1
    };

    // Vertex vertices[] = {
    //     {{{ 0.5f,  -0.5f, 0.5f  }}, { 255, 0,   0,   255 }},
    //     {{{ 0.5f,  0.5f,  0.5f  }}, { 0,   255, 0,   255 }},
    //     {{{ -0.5f, 0.5f,  0.5f  }}, { 0,   0,   255, 255 }},
    //     {{{ -0.5f, -0.5f, 0.5f  }}, { 255, 255, 0,   255 }},
    //     {{{ 0.5f,  -0.5f, -0.5f }}, { 255, 0,   0,   255 }},
    //     {{{ 0.5f,  0.5f,  -0.5f }}, { 0,   255, 0,   255 }},
    //     {{{ -0.5f, 0.5f,  -0.5f }}, { 0,   0,   255, 255 }},
    //     {{{ -0.5f, -0.5f, -0.5f }}, { 255, 255, 0,   255 }},
    // };

    // u16 indices[] = {
    //     0, 1, 2,
    //     0, 2, 3,
    //     4, 5, 6,
    //     4, 6, 7,
    //     4, 5, 1,
    //     4, 1, 0,
    //     3, 2, 6,
    //     3, 6, 7,
    //     4, 0, 3,
    //     4, 3, 7,
    //     6, 2, 1,
    //     6, 1, 5
    // };  

    shape2d_make(vertices, indices, sizeof(vertices) / sizeof(Vertex), sizeof(indices) / sizeof(u16), &Shape);

    render_set_active_camera(&Cam);

    u64 last_time = glass_query_performance_counter();
    u64 current_time = 0;

    u64 max_fps           = 1000;
    // float dt_float = 0;

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
        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_ESCAPE)) {
            glass_exit();
            break;
        }

        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_UP)) {
            Target_Fps += 1;
            Target_Fps = clamp(Target_Fps, 1ull, max_fps);
        } else if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_DOWN)) {
            Target_Fps -= 1;
            Target_Fps = clamp(Target_Fps, 1ull, max_fps);
        }

        u64 target_frame_time = 1000 / Target_Fps;

        if (glass_exit_required()) {
            glass_exit();
            break;
        }

        // Matrix4 view = matrix4_camera_view_2d(Cam.position);
        // Matrix4 proj = matrix4_ortho_2d(Cam.left, Cam.right, Cam.top, Cam.bottom);

        Matrix4 view = matrix4_view(Cam.position, Cam.rotation);
        Matrix4 proj = matrix4_perspective(Cam.fov, Cam.aspect, Cam.near_plane, Cam.far_plane);

        render_set_camera_matrices(view, proj, Cam.position);
        render_set_time(G_Context.time.dt, G_Context.time.time);

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

        G_Context.time.dt_double    = dt;
        G_Context.time.dt           = (float)dt;
        G_Context.time.time_double += dt;
        G_Context.time.time         = (float)G_Context.time.time_double;

        u64 fps = (u64)(1.0l / dt);

        char buf[1024];

        sprintf(buf, "fps: %llu, dt:%f, time:%f\n", fps, G_Context.time.dt, G_Context.time.time);
        glass_set_window_title(G_Context.wnd, buf);
    }

    return 0;
}

void glass_exit() {
    render_destroy();

    glass_destroy_all_windows();
}

GlassErrorCode glass_render(Window* window) {
    clear_color_buffer(Vector4(0.3f, 0.3f, 0.1f, 1.0f));

    render_shape_2d(Active_Material, &Shape, &Test_Transform);

    // Logf("Archetypes count: %d", em.archetypes.count);

    // for (auto [archetype, list] : em.archetypes) {
    //     printf("Archetype: ");
    //     bitmap_print(archetype);
    //     printf("Entities: ");

    //     for(auto entity : list) {
    //         printf("%d, ", entity);
    //     }
    //     printf("\n");
    // }

    EntityManager* emp = &em;

    BEGIN_ITERATE_COMPONENTS_2(emp, Transform, Renderer2D)
    render_shape_2d(Renderer2D_c->material, Renderer2D_c->shape, Transform_c);
    END_ITERATE_COMPONENTS_2()

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

Queue<EntityHandle> Entities = queue_make<EntityHandle>();

void glass_game_code() {
    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_ALPHA1)) {
        Target_Fps = 1;
    }

    const float speed = 2.0f;
    const float cam_speed = 3.0f;
    const float angular_speed = 5.0f;
    const float zoom_speed = 3.0f;
    const float zoom_mult = 5.0f;

    Vector3 direction = {};
    Vector3 cam_direction = {};

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_W)) {
        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_LEFT_CONTROL)) {
            direction.z = 1.0f;
        } else {
            direction.y = 1.0f;
        }
    } else if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_S)) {
        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_LEFT_CONTROL)) {
            direction.z = -1.0f;
        } else {
            direction.y = -1.0f;
        }
    }

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_D)) {
        direction.x = 1.0f;
    }else if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_A)) {
        direction.x = -1.0f;
    }

    Test_Transform.position += direction * (G_Context.time.dt * speed);

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_B)) {
        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_LEFT_CONTROL)) 
            Transform_Rotation.x += angular_speed * G_Context.time.dt;
        else
            Camera_Rotation.x += angular_speed * G_Context.time.dt;
    }
    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_N)) {
        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_LEFT_CONTROL)) 
            Transform_Rotation.y += angular_speed * G_Context.time.dt;
        else 
            Camera_Rotation.y += angular_speed * G_Context.time.dt;
    }
    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_M)) {
        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_LEFT_CONTROL))
            Transform_Rotation.z += angular_speed * G_Context.time.dt;
        else
            Camera_Rotation.z += angular_speed * G_Context.time.dt;
    }

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_I)) {
        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_LEFT_CONTROL)) {
            cam_direction.z = 1.0f;
        } else {
            cam_direction.y = 1.0f;
        }
    } else if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_K)) {
        if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_LEFT_CONTROL)) {
            cam_direction.z = -1.0f;
        } else {
            cam_direction.y = -1.0f;
        }
    }

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_L)) {
        cam_direction.x = 1.0f;
    }else if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_J)) {
        cam_direction.x = -1.0f;
    }

    float zoom = zoom_speed;

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_LEFT_SHIFT)) {
        zoom *= zoom_mult;
    }

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_U)) {
        Cam.size -= zoom * G_Context.time.dt;
    } else if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_O)) {
        Cam.size += zoom * G_Context.time.dt;
    }

    Cam.size = clamp(Cam.size, 0.01f, 100.0f);

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_R)) {
        Cam.size = 5.0f;
    }

    // Vector3 cam_pos = Cam.position;

    Cam.position += cam_direction * (cam_speed * G_Context.time.dt);
    Cam.rotation = quaternion_euler(Camera_Rotation);
    // camera_update_ortho(&Cam);
    camera_perspective_update_position(&Cam, Cam.position);
    camera_perspective_update_rotation(&Cam, Cam.rotation);

    Test_Transform.rotation = quaternion_euler(Transform_Rotation);
    // Logf("Cam position: %f, %f, %f, rotation: %f", Cam.position.x, Cam.position.y, Cam.position.z, Cam.rotation);

    const Vector3 min_pos = Vector3 {
        .x = -30.0f,
        .y = -30.0f,
        .z = -30.0f
    };

    const Vector3 max_pos = Vector3 {
        .x = 30.0f,
        .y = 30.0f,
        .z = 30.0f
    };

    const Vector3 min_scale = Vector3 {
        .x = 0.2f,
        .y = 0.2f,
        .z = 0.2f
    };

    const Vector3 max_scale = Vector3 {
        .x = 5.0f,
        .y = 5.0f,
        .z = 5.0f
    };

    EntityManager* e = &em;

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_SPACE)) {
        EntityHandle ent = entity_create(&em);

        Transform trans = {
            .position = vector3_random(min_pos, max_pos),
            .rotation = quaternion_angle_axis(radians(frand(-180.0f, 180.0f)), vector3_forward),
            .scale    = vector3_random(min_scale, max_scale),
        };

        TestComponent test = {
            .a = 2,
            .b = 3
        };

        TestComponent2 test2 = {
            .c = 2,
        };

        Renderer2D renderer = {
            .shape    = &Shape,
            .material = Active_Material
        };

        ADD_COMPONENT(Transform, e, ent.id, trans);
        ADD_COMPONENT(TestComponent, e, ent.id, test);
        ADD_COMPONENT(Renderer2D, e, ent.id, renderer);
        ADD_COMPONENT(TestComponent2, e, ent.id, test2);
        
        queue_enqueue(&Entities, ent);
    }

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_T)) {
        if (Entities.count > 0) {
            EntityHandle ent = queue_dequeue(&Entities);
            entity_destroy(e, ent);
            Logf("Removed %d", ent.id);
        }
    }

    if (glass_is_button_pressed(G_Context.wnd, GLASS_SCANCODE_Z)) {
        if (Entities.count > 0) {
            for(auto ent : Entities) {
                if (HAS_COMPONENT(TestComponent2, e, ent.id)) {
                    entity_print_components(e, ent.id);
                    REMOVE_COMPONENT(TestComponent2, e, ent.id);
                    break;
                }
            }
        }
    }
}