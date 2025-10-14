#version 450

layout(binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 proj;
} vp;

layout(binding = 1) uniform Model {
    mat4 model;
} m;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = vp.proj * vp.view * m.model * vec4(inPosition, 0.0, 1.0);
    fragColor   = inColor;
}