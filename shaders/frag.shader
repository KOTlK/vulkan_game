#version 460 core

layout (location = 0) in vec4 v2f_color;

out vec4 out_color;

void main() {
    out_color = v2f_color;
}