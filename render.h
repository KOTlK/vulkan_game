#pragma once

#include "types.h"
#include "geometry.h"
#include "Vector2.h"
#include "Vector3.h"
#include "game_context.h"
#include "text.h"
#include "Matrix4.h"
#include "Vector4.h"

struct Window;
struct Shader;
struct Material;

struct Camera {
    Vector3 position;
    float   rotation;
    float   size;
    float   aspect;
    float   left;
    float   right;
    float   top;
    float   bottom;
};

struct Transform {
    Vector3 position;
    Vector3 scale;
    float   rotation;
};

struct Renderer2D {
    Shape2D*  shape;
    Material* material;
};

enum RenderError {
    RENDER_OK                       = 0,
    RENDER_INTERNAL_ERROR           = 1,
    RENDER_NO_GPU_FOUND             = 2,
    RENDER_INCOMPATIBLE_SHADER_DATA = 3,
};

RenderError render_init(Context* context);
void        render_destroy();
RenderError render_test();

void render_set_active_camera(Camera* cam);
void render_set_camera_matrices(Matrix4 v, Matrix4 p, Vector3 pos);
void render_set_time(float dt, float time);

void        clear_color_buffer(Vector4 color);
RenderError render_shape_2d(Material* mat, Shape2D* shape, Transform* transform);

void material_set_matrix(Material* mat, String name, Matrix4 data);
void material_set_matrix(Material* mat, u32 location, Matrix4 data);

s32 material_get_uniform_location(Material* mat, String name);

Shader* shader_make(String* vert, String* frag, RenderError* err);
void    shader_destroy(Shader* shader);

Material* material_make(Shader* shader);
void      material_destroy(Material* mat);

// camera
extern void camera_make_ortho(Vector3 position, float rotation, float size, float aspect_ratio, Camera* camera);
extern void camera_update_position(Camera* cam, Vector3 position);
extern void camera_update_size(Camera* cam, float size);
extern void camera_update_ortho(Camera* cam);