#version 450
#extension GL_EXT_nonuniform_qualifier : require

#include "common.glsl"

struct ObjectData {
    mat4 model;
    mat4 mvp;
};

layout(set = 1, binding = 0) readonly buffer PerObjectData {
    ObjectData objects[];
} per_object;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    mat4 mvp    = per_object.objects[gl_InstanceIndex].mvp;
    gl_Position = mvp * vec4(inPosition, 0.0, 1.0);
    fragColor   = inColor;
}