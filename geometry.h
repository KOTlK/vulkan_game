#pragma once

#include "basic.h"
#include "Vector2.h"

typedef struct Color {
    u8 r, g, b, a;
} Color;

typedef struct Vertex2D {
    Vector2 position;
    Color   color;
} Vertex2D;

typedef struct Shape2D {
    Vertex2D* vertices;
    u16*      indices;
    u32       vertex_count;
    u32       index_count;
} Shape2D;

void shape2d_make(Vertex2D* vertices, u16* indices, u32 vertex_count, u32 index_count, Shape2D* shape);
void shape2d_free(Shape2D* shape);