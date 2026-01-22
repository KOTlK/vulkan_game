export module vector4;

export union Vector4 {
    struct {float x, y, z, w;};
    struct {float r, g, b, a;};
    float e[4];

    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {
    }
};