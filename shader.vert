#version 450

layout(binding = 0) uniform ViewProjection {
    mat4 view;
    mat4 proj;
} view_projection;

layout(binding = 1) uniform Model {
    mat4 model;
} model;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    // mat4 vp  = view_projection.view * view_projection.proj;
    // mat4 mvp = model.model * vp;
    mat4 mvp = view_projection.proj * view_projection.view * model.model;

    gl_Position = mvp * vec4(inPosition, 0.0, 1.0);
    fragColor   = inColor;
}