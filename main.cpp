#define GAME_MATH_IMPLEMENTATION
#define TEXT_IMPLEMENTATION
#define FILE_IMPLEMENTATION
#define BITMAP_IMPLEMENTATION
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
#include "components.h"
// #include "entities.h"
#include "component_system.h"
#include "list.h"
#include "queue.h"
#include "Vector4.h"
#include "text.h"
#include "file.h"
#include "bitmap.h"

#define WIDTH  1280
#define HEIGHT 720

Context Game_Context{};

Matrix4 VIEW;
Matrix4 PROJECTION;

Camera  Cam;

static EntityManager em;
static Material* Active_Material;
static Shape2D   Shape;

static Transform Test_Transform = {
    .position = {{0, 0, 0}},
    .scale    = {{1, 1, 1}},
    .rotation = radians(0.0f)
};

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

    Game_Context.wnd = glass_create_window(400, 100, WIDTH, HEIGHT, name, &err);


    if (err != GLASS_OK) {
        Errf("Cannot create window. %d", err);
        return 1;
    }

    Log("Window created.");

    float aspect_ratio = (float)WIDTH / (float)HEIGHT;

    camera_make_ortho(vector3_make(0,0,0), 0, 10, aspect_ratio, &Cam);

    RenderError render_err = render_init(&Game_Context);

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
    Vertex vertices[] = {
        {{{   0.5f,  -0.5f, 0.0f }}, { 255, 0,   0,   255 }},
        {{{   0.5f,   0.5f, 0.0f }}, { 0,   255, 0,   255 }},
        {{{  -0.5f,   0.5f, 0.0f }}, { 0,   0,   255, 255 }},
        {{{  -0.5f,  -0.5f, 0.0f }}, { 255, 255, 0, 255 }},
    };

    u16 indices[] = {
        0, 1, 2,
        0, 2, 3
    };  

    shape2d_make(vertices, indices, sizeof(vertices) / sizeof(Vertex), sizeof(indices) / sizeof(u16), &Shape);

    render_set_active_camera(&Cam);

    u64 last_time = glass_query_performance_counter();
    u64 current_time = 0;

    u64 max_fps           = 1000;
    float dt_float = 0;

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
            Target_Fps += 1;
            Target_Fps = clamp(Target_Fps, 1ull, max_fps);
        } else if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_DOWN)) {
            Target_Fps -= 1;
            Target_Fps = clamp(Target_Fps, 1ull, max_fps);
        }

        u64 target_frame_time = 1000 / Target_Fps;

        if (glass_exit_required())
            break;

        Matrix4 view = matrix4_camera_view_2d(Cam.position);
        Matrix4 proj = matrix4_ortho_2d(Cam.left, Cam.right, Cam.top, Cam.bottom);

        render_set_camera_matrices(view, proj, Cam.position);
        render_set_time(Game_Context.time.dt, Game_Context.time.time);

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

        char buf[1024];

        sprintf(buf, "fps: %llu, dt:%f, time:%f\n", fps, Game_Context.time.dt, Game_Context.time.time);
        glass_set_window_title(Game_Context.wnd, buf);
    }

    render_destroy();
    glass_destroy_window(Game_Context.wnd);

    return 0;
}

GlassErrorCode glass_render(Window* window) {
    clear_color_buffer(Vector4(0.3f, 0.3f, 0.1f, 1.0f));

    render_shape_2d(Active_Material, &Shape, &Test_Transform);

    BEGIN_ITERATE_COMPONENT(Transform)
    render_shape_2d(Active_Material, &Shape, component);
    END_ITERATE_COMPONENT()

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
    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_ALPHA1)) {
        Target_Fps = 1;
    }

    const float speed = 2.0f;
    const float cam_speed = 3.0f;
    const float angular_speed = 5.0f;
    const float zoom_speed = 3.0f;
    const float zoom_mult = 5.0f;

    Vector3 direction = {};
    Vector3 cam_direction = {};

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_W)) {
        direction.y = 1.0f;
    } else if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_S)) {
        direction.y = -1.0f;
    }

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_D)) {
        direction.x = 1.0f;
    }else if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_A)) {
        direction.x = -1.0f;
    }

    Test_Transform.position += direction * (Game_Context.time.dt * speed);

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_Q)) {
        Test_Transform.rotation += angular_speed * Game_Context.time.dt;
    } else if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_E)) {
        Test_Transform.rotation -= angular_speed * Game_Context.time.dt;
    }

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_I)) {
        cam_direction.y = 1.0f;
    } else if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_K)) {
        cam_direction.y = -1.0f;
    }

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_L)) {
        cam_direction.x = 1.0f;
    }else if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_J)) {
        cam_direction.x = -1.0f;
    }

    float zoom = zoom_speed;

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_LEFT_SHIFT)) {
        zoom *= zoom_mult;
    }

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_U)) {
        Cam.size -= zoom * Game_Context.time.dt;
    } else if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_O)) {
        Cam.size += zoom * Game_Context.time.dt;
    }

    Cam.size = clamp(Cam.size, 0.01f, 100.0f);

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_R)) {
        Cam.size = 5.0f;
    }

    // Vector3 cam_pos = Cam.position;

    Cam.position += cam_direction * (cam_speed * Game_Context.time.dt);

    camera_update_ortho(&Cam);
    // Logf("Cam position: %f, %f, %f, rotation: %f", Cam.position.x, Cam.position.y, Cam.position.z, Cam.rotation);

    const Vector3 min_pos = Vector3 {
        .x = -30.0f,
        .y = -30.0f,
        .z = 0
    };

    const Vector3 max_pos = Vector3 {
        .x = 30.0f,
        .y = 30.0f,
        .z = 0
    };

    const Vector3 min_scale = Vector3 {
        .x = -2.0f,
        .y = -2.0f,
        .z = 1
    };

    const Vector3 max_scale = Vector3 {
        .x = 2.0f,
        .y = 2.0f,
        .z = 1
    };

    EntityManager* e = &em;

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_SPACE)) {
        EntityHandle ent = entity_create(&em);

        Transform trans = {
            .position = vector3_random(min_pos, max_pos),
            .scale    = vector3_random(min_scale, max_scale),
            .rotation = radians(frand(-180.0f, 180.0f)),
        };

        ADD_COMPONENT(Transform, e, ent, trans);
        
        queue_enqueue(&Entities, ent);
    }

    if (glass_is_button_pressed(Game_Context.wnd, GLASS_SCANCODE_T)) {
        if (Entities.count > 0) {
            EntityHandle ent = queue_dequeue(&Entities);
            entity_destroy(e, ent);
            // REMOVE_COMPONENT(Transform, e, ent);
            Logf("Removed %d", ent.id);
        }
    }
}