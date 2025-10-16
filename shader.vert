#version 450

layout(binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

layout(binding = 1) uniform Model {
    mat4 model;
    mat4 mvp;
} model;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    // mat4 mvp = camera.proj * camera.view * model.model;
    
    gl_Position = model.mvp * vec4(inPosition, 0.0, 1.0);
    fragColor   = inColor;
}