#pragma once

static inline
u64
get_hash(u8 i) {
    return 1 + i ^ 33;
}

static inline
u64
get_hash(s8 i) {
    return 1 + i ^ 33;
}

static inline
u64
get_hash(u16 i) {
    return 1 + i ^ 234656;
}

static inline
u64
get_hash(s16 i) {
    return 1 + i ^ 982374;
}

static inline
u64
get_hash(u32 i) {
    return 1 + i ^ 109238123;
}

static inline
u64
get_hash(s32 i) {
    return 1 + i ^ 98672453;
}

static inline
u64
get_hash(u64 i) {
    return 1 + i ^ 17862943124;
}

static inline
u64
get_hash(s64 i) {
    return 1 + i ^ 9875612353;
}

static inline
u64 get_hash(char* str) {
    u64 hash = 5381;
    s32 c;

    while((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

template <typename T>
static inline
u64 get_hash(T* ptr) {
    return (u64)ptr;
}