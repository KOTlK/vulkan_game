#pragma once

#include "Vector2.h"

typedef struct Transform2D {
    Vector2 position;
    Vector2 scale;
    float   rotation;
} Transform2D;