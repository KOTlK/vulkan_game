#pragma once

inline
u64
get_hash(u8 i) {
    return 1 + i ^ 33;
}

inline
u64
get_hash(s8 i) {
    return 1 + i ^ 33;
}

inline
u64
get_hash(u16 i) {
    return 1 + i ^ 234656;
}

inline
u64
get_hash(s16 i) {
    return 1 + i ^ 982374;
}

inline
u64
get_hash(u32 i) {
    return 1 + i ^ 109238123;
}

inline
u64
get_hash(s32 i) {
    return 1 + i ^ 98672453;
}

inline
u64
get_hash(u64 i) {
    return 1 + i ^ 17862943124;
}

inline
u64
get_hash(s64 i) {
    return 1 + i ^ 9875612353;
}

inline
u64 get_hash(char* str) {
    u64 hash = 5381;
    s32 c;

    while((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

template <typename T>
inline
u64 get_hash(T* ptr) {
    return (u64)ptr;
}