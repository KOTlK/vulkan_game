#pragma once

#include "basic.h"
#include "allocator.h"
#include <malloc.h>

static inline
void*
std_alloc(Allocator* allocator, u64 size) {
    return malloc(size);
}

static inline
void*
std_realloc(Allocator* allocator, void* ptr, u64 size) {
    return realloc(ptr, size);
}

static inline
void
std_free(Allocator* allocator, void* ptr) {
    free(ptr);
}