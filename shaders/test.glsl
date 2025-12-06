#version 450

#include "common.glsl"

// struct ObjectData {
//     mat4 model;
//     mat4 mvp;
// };

layout(set = 1, binding = 0) uniform ObjectData {
    mat4 model;
    mat4 mvp;
} object_data;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = object_data.mvp * vec4(inPosition, 0.0, 1.0);
    fragColor   = inColor;
}