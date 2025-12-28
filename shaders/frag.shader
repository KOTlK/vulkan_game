#version 460 core

layout (location = 0) in vec4 v2f_color;

layout(std140, binding = 1) uniform Time {
   float dt;
   float time;
   float sin_time;
   float cos_time;
} time;

vec4 second_color = vec4(0.3, 0.3, 0.3, 1);

out vec4 out_color;

void main() {
    out_color = mix(v2f_color, second_color, time.sin_time);
}