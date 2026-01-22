module;

#include "types.h"

export module math;

#ifdef min
#undef min 
#endif

#ifdef max
#undef max 
#endif

export constexpr float FLOAT_EPSILON = 1e-5f;
export constexpr float PI            = 3.141592653f;

export inline float degrees(float rad) {return rad * 57.2957795f;}
export inline float radians(float deg) {return deg * 0.0174532925f;}

export template <typename T>
inline T clamp(T value, T min, T max) {
    return value < min ? min : (value > max ? max : value);
}

export template <typename T>
inline T min(T lhs, T rhs) {
    return lhs < rhs ? lhs : rhs;
}

export template <typename T>
inline T max(T lhs, T rhs) {
    return lhs > rhs ? lhs : rhs;
}

export inline u32 next_power_of_2(u32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

export template <typename T>
inline void swap(T* a, T* b) {
    T temp = *a;
    *a = *b;
    *b = temp;
}