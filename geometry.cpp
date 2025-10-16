#include "geometry.h"
#include <memory.h>

void shape2d_make(float* positions, Color* colors, u32 vertex_count, Allocator* allocator, Shape2D* shape) {
    u32 size = sizeof(float) * 2 * vertex_count + sizeof(Color) * vertex_count;
    char* data          = (char*)allocator_alloc(allocator, size);
    u32   colors_offset = sizeof(float) * 2 * vertex_count;

    memcpy(data, positions, colors_offset);
    memcpy(&data[colors_offset], colors, sizeof(Color) * vertex_count);

    shape->vertices     = (Vertex2D*)data;
    shape->colors       = (Color*)(data + colors_offset);
    shape->vertex_count = vertex_count;
}

void shape2d_free(Shape2D* shape, Allocator* allocator) {
    allocator_free(allocator, shape->vertices);
}