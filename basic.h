#pragma once

#include <malloc.h>
#include "types.h"

#define null NULL
#define Malloc(type, size) (type*)malloc(size)
#define Calloc(type, count) (type*)malloc(sizeof(type) * count)
#define Realloc(type, ptr, new_size) (type*)realloc(ptr, new_size)
#define Make(type) (type*)malloc(sizeof(type))
#define Free(mem) free(mem)

#include "allocator.h"
#include "std_allocator.h"
#include "arena.h"

static Allocator* Allocator_Persistent = new AllocatorPersistent();
static Allocator* Allocator_Temp       = new Arena();

static inline
Allocator*
get_persistent_allocator() {
    return Allocator_Persistent;
}

static inline
Allocator*
get_temp_allocator() {
    return Allocator_Temp;
}

static inline
void
free_temp_allocator() {
    Allocator_Temp->clear();
}