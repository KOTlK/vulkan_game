#pragma once

#include "basic.h"

struct Allocator;

typedef void* (*AllocateFn)(Allocator* allocator, u64 size);
typedef void* (*ReallocateFn)(Allocator* allocator, void* ptr, u64 new_size);
typedef void  (*FreeFn)(Allocator* allocator, void* ptr);

struct Allocator {
    AllocateFn   alloc;
    ReallocateFn realloc;
    FreeFn       free;
    void        *context;
};

static inline
void*
allocator_alloc(Allocator *allocator, u64 size) {
    return allocator->alloc(allocator, size);
}

static inline
void*
allocator_realloc(Allocator *allocator, void* ptr, u64 size) {
    return allocator->realloc(allocator, ptr, size);
}

static inline
void
allocator_free(Allocator *allocator, void* ptr) {
    return allocator->free(allocator, ptr);
}