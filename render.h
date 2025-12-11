#pragma once

#include "basic.h"
#include "geometry.h"
#include "Vector2.h"
#include "game_context.h"

struct Window;
struct Shader;
struct Material;
struct CommandBuffer;

typedef struct Camera {
    Vector2 position;
    float   rotation;
    float   size;
    float   aspect;
    float   left;
    float   right;
    float   top;
    float   bottom;
} Camera;

typedef enum RenderError {
    RENDER_OK             = 0,
    RENDER_INTERNAL_ERROR = 1,
    RENDER_NO_GPU_FOUND   = 2,
} RenderError;

RenderError render_init(Context* context);
void        render_destroy();
RenderError render_test();

RenderError create_command_buffer(CommandBuffer* cmd);
void        cmd_begin_render(CommandBuffer* cmd);
void        cmd_begin_command_buffer(CommandBuffer* cmd);
void        free_command_buffer(CommandBuffer* cmd);

Shader* shader_make(char* vert_text, char* frag_text, u32 vert_len, u32 frag_len, RenderError* err);
void    shader_destroy(Shader* shader);

RenderError render_vertices_indexed16(CommandBuffer* cmd, Material* shader, Vertex2D* vertices, u16* indices, u32 vertex_count, u32 index_count);

// camera
void camera_make_ortho(float size, float aspect_ratio, Camera* camera);
void camera_update_position_and_rotation(Camera* cam, Vector2 position, float rotation);