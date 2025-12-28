#version 460 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec4 in_color;

layout(std140, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    mat4 view_proj;
    vec3 pos;
} camera;

layout(std140, binding = 1) uniform Time {
   float dt;
   float time;
   float sin_time;
   float cos_time;
} time;

uniform mat4 model;
uniform mat4 mvp;

layout (location = 0) out vec4 v2f_color;

void main() {
   v2f_color = in_color;

   mat4 m = model;

   // gl_Position = vec4(in_pos.x, in_pos.y, in_pos.z, 1.0);

   gl_Position = mvp * vec4(in_pos, 1.0);
}