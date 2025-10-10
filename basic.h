#pragma once

#include <malloc.h>

#define null NULL
#define Malloc(type, size) (type*)malloc(size)
#define Calloc(type, count) (type*)malloc(sizeof(type) * count)
#define Realloc(type, ptr, new_size) (type*)realloc(ptr, new_size)
#define Free(mem) free(mem)

#include <stdint.h>

#define s8  int8_t
#define u8  uint8_t
#define s16 int16_t
#define u16 uint16_t
#define s32 int32_t
#define u32 uint32_t
#define s64 int64_t
#define u64 uint64_t

#define u8_max  UINT8_MAX
#define u16_max UINT16_MAX
#define u32_max UINT32_MAX
#define u64_max UINT64_MAX
#define s8_max  INT8_MAX
#define s16_max INT16_MAX
#define s32_max INT32_MAX
#define s64_max INT64_MAX
#define s8_min  INT8_MIN
#define s16_min INT16_MIN
#define s32_min INT32_MIN
#define s64_min INT64_MIN

#include "allocator.h"
#include "std_allocator.h"
#include "arena.h"

static Allocator Allocator_Std = {
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
get_std_allocator() {
    return &Allocator_Std;
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