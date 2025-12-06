#pragma once

#include <malloc.h>
#include "types.h"

#define null NULL
#define Malloc(type, size) (type*)malloc(size)
#define Calloc(type, count) (type*)malloc(sizeof(type) * count)
#define Realloc(type, ptr, new_size) (type*)realloc(ptr, new_size)
#define Make(type) (type*)malloc(sizeof(type))
#define Free(mem) free(mem)

#define AllocatorAlloc(type, allocator, size) (type*)allocator_alloc(allocator, size)
#define AllocatorCalloc(type, allocator, count) (type*)allocator_alloc(allocator, sizeof(type) * (count))
#define AllocatorFree(allocator, data) allocator_free(allocator, data)

#include "allocator.h"
#include "std_allocator.h"
#include "arena.h"

static Allocator Allocator_Persistent = {
    .alloc   = std_alloc,
    .realloc = std_realloc,
    .free    = std_free,
    .context = null
};

static Allocator Allocator_Temp = {
    .alloc   = arena_alloc,
    .realloc = arena_realloc,
    .free    = arena_free,
    .context = arena_make()
};

static inline
Allocator*
get_persistent_allocator() {
    return &Allocator_Persistent;
}

static inline
Allocator*
get_temp_allocator() {
    return &Allocator_Temp;
}

static inline
void
free_temp_allocator() {
    Allocator_Temp.free(&Allocator_Temp, null);
}