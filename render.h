#pragma once

#include "basic.h"
#include "geometry.h"
#include "Vector2.h"
#include "game_context.h"
#include "text.h"
#include "Matrix4.h"

struct Window;
struct Shader;
struct Material;
// struct CommandBuffer;

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

RenderError render_shape_2d(Material* mat, Shape2D* shape, Matrix4 trs);

Shader* shader_make(String* vert, String* frag, RenderError* err);
void    shader_destroy(Shader* shader);

Material* material_make(Shader* shader);
void      material_destroy(Material* mat);

// camera
void camera_make_ortho(float size, float aspect_ratio, Camera* camera);
void camera_update_position_and_rotation(Camera* cam, Vector2 position, float rotation);