#include "geometry.h"
#include "types.h"
#include <memory.h>

void shape2d_make(Vertex* vertices, u16* indices, u32 vertex_count, u32 index_count, Shape2D* shape) {
    u32   size = sizeof(Vertex) * vertex_count + sizeof(u16) * index_count;
    char* data = Malloc(char, size);
    u32   index_offset = sizeof(Vertex) * vertex_count;

    memcpy(data, vertices, index_offset);
    memcpy(&data[index_offset], indices, sizeof(u16) * index_count);

    shape->vertices     = (Vertex*)data;
    shape->indices      = (u16*)(data + index_offset);
    shape->vertex_count = vertex_count;
    shape->index_count  = index_count;
}

void shape2d_free(Shape2D* shape) {
    Free(shape->vertices);
}