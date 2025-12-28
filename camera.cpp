#include "render.h"

void camera_make_ortho(Vector3 position, float rotation, float size, float aspect_ratio, Camera* camera) {
    float half_size  = size * 0.5f;
    camera->position = position;
    camera->rotation = rotation;
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

// void camera_update_position_and_rotation(Camera* cam, Vector3 position, float rotation) {
//     float half_size = cam->size * 0.5f;
//     cam->position   = position;
//     cam->rotation   = rotation;
//     cam->left       = position.x - half_size * cam->aspect;
//     cam->right      = position.x + half_size * cam->aspect;
//     cam->top        = position.y - half_size;
//     cam->bottom     = position.y + half_size;
// }