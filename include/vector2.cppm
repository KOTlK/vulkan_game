module;

export module vector2;

export union Vector2 {
    struct {
        float x, y;
    };
    float e[2];
};

export inline Vector2 vector2_make(float x, float y) {
    Vector2 v = {
        .x = x,
        .y = y
    };

    return v;
}

export inline float   dot  (Vector2 lhs, Vector2 rhs) {
    return lhs.x * rhs.x +
           lhs.y * rhs.y;
}

export inline float cross(Vector2 lhs, Vector2 rhs) {
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

export inline Vector2 clamp(Vector2 value, Vector2 min, Vector2 max) {
    return {
        .x = value.x < min.x ? min.x : (value.x > max.x ? max.x : value.x),
        .y = value.y < min.y ? min.y : (value.y > max.y ? max.y : value.y),
    };
}