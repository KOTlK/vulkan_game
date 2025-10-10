#pragma once

#include "basic.h"
#include "allocator.h"
#include "assert.h"

#define USE_MALLOC

#ifdef USE_MALLOC
    #include "malloc.h"
    #define Arena_Malloc(size)       malloc(size)
    #define Arena_Realloc(ptr, size) realloc(ptr, size)
    #define Arena_Free(ptr)          free(ptr)
#else
    #define Arena_Malloc(size)
    #define Arena_Free(ptr)
#endif

// 8MB size by default
#define ARENA_INITIAL_CAPACITY 1024 * 1024 * 8
// resize by 1MB
#define ARENA_RESIZE_STEP 1024 * 1024

struct Arena {
    u8* data;
    u64 capacity;
    u64 allocated;
};

static inline
void
arena_resize(Arena* arena, u64 size);

static inline
Arena*
arena_make(u64 initial_capacity = ARENA_INITIAL_CAPACITY) {
    Arena* arena = (Arena*)Arena_Malloc(sizeof(Arena));
    Assert(arena, "Cannot allocate arena.");
    u8* data = (u8*)Arena_Malloc(initial_capacity);
    Assert(data, "Cannot allocate arena's data.");

    arena->data      = data;
    arena->capacity  = initial_capacity;
    arena->allocated = 0;

    return arena;
}

static inline
void*
arena_alloc(Allocator* allocator, u64 size) {
    Arena* arena = (Arena*)allocator->context;
    Assert(arena, "Cannot allocate data, because arena is null.");

    if (arena->capacity < arena->allocated + size) {
        arena_resize(arena, arena->allocated + size + ARENA_RESIZE_STEP);
    }

    void* ptr = &arena->data[arena->allocated];

    arena->allocated += size;

    return ptr;
}

static inline
void*
arena_realloc(Allocator* allocator, void* ptr, u64 size) {
    Assert(false, "Cannot realloc data, using arena allocator");
    return null;
}

static inline
void
arena_free(Allocator* allocator, void* ptr) {
    Arena* arena = (Arena*)allocator->context;
    arena->allocated = 0;
}

static inline
void
arena_resize(Arena* arena, u64 size) {
    arena->data = (u8*)Arena_Realloc(arena->data, size);
    Assert(arena->data, "Cannot resize arena.");
    arena->capacity = size;
}