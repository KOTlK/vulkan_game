#pragma once

#include "types.h"
#include "Vector3.h"

typedef struct Color {
    u8 r, g, b, a;
} Color;

typedef struct Vertex {
    Vector3 position;
    Color   color;
} Vertex;

typedef struct Shape2D {
    Vertex* vertices;
    u16*      indices;
    u32       vertex_count;
    u32       index_count;
} Shape2D;

void shape2d_make(Vertex* vertices, u16* indices, u32 vertex_count, u32 index_count, Shape2D* shape);
void shape2d_free(Shape2D* shape);