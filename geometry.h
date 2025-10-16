#pragma once

#include "basic.h"
#include "Vector2.h"

typedef struct Color {
    u8 r, g, b, a;
} Color;

typedef struct Vertex2D {
    Vector2 position;
} Vertex2D;

typedef struct Shape2D {
    Vertex2D* vertices;
    Color*    colors;
    u32       vertex_count;
} Shape2D;

// make 2d shape. positions - [xyxyxyxyxy]
void shape2d_make(float* positions, Color* colors, u32 vertex_count, Allocator* allocator, Shape2D* shape);
void shape2d_free(Shape2D* shape, Allocator* allocator);