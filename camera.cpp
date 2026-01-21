#define GAME_MATH_IMPLEMENTATION
#include "render.h"
#include "assert.h"

void camera_make_ortho(Vector3 position, float rotation, float size, float aspect_ratio, Camera* camera) {
    float half_size  = size * 0.5f;
    camera->position = position;
    // camera->rotation = rotation;
    camera->rotation = quaternion_angle_axis(rotation, vector3_forward);
    camera->size     = size;
    camera->aspect   = aspect_ratio;
    camera->left     = position.x - half_size * aspect_ratio;
    camera->right    = position.x + half_size * aspect_ratio;
    camera->top      = position.y - half_size;
    camera->bottom   = position.y + half_size;
}

void camera_update_position(Camera* cam, Vector3 position) {
    float half_size = cam->size * 0.5f;
    cam->position   = position;
    cam->left       = position.x - half_size * cam->aspect;
    cam->right      = position.x + half_size * cam->aspect;
    cam->top        = position.y - half_size;
    cam->bottom     = position.y + half_size;
}

void camera_update_size(Camera* cam, float size) {
    cam->size = size;

    camera_update_position(cam, cam->position);
}

void camera_update_ortho(Camera* cam) {
    float   half_size = cam->size * 0.5f;
    Vector3 position  = cam->position;
    cam->left       = position.x - half_size * cam->aspect;
    cam->right      = position.x + half_size * cam->aspect;
    cam->top        = position.y - half_size;
    cam->bottom     = position.y + half_size;
}

void camera_make_perspective(const Vector3& p, 
                             const Quaternion& r, 
                             float aspect,
                             float fov,
                             float near_plane,
                             float far_plane, 
                             Camera* cam) {
    Assert(cam, "cam is NULL.");

    cam->position   = p;
    cam->rotation   = r;
    cam->aspect     = aspect;
    cam->near_plane = near_plane;
    cam->far_plane  = far_plane;
    cam->fov        = fov;
}

void camera_perspective_update_position(Camera* cam, const Vector3& p) {
    cam->position = p;
}

void camera_perspective_update_rotation(Camera* cam, const Quaternion& r) {
    cam->rotation = r;
}

void camera_perspective_set_fov(Camera* cam, float fov) {
    cam->fov = fov;
}

// void camera_update_position_and_rotation(Camera* cam, Vector3 position, float rotation) {
//     float half_size = cam->size * 0.5f;
//     cam->position   = position;
//     cam->rotation   = rotation;
//     cam->left       = position.x - half_size * cam->aspect;
//     cam->right      = position.x + half_size * cam->aspect;
//     cam->top        = position.y - half_size;
//     cam->bottom     = position.y + half_size;
// }