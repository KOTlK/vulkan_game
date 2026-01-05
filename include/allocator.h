#pragma once

#include "types.h"

#define AllocatorAlloc(type, allocator, size) (type*)(allocator)->alloc(size)
#define AllocatorCalloc(type, allocator, count) (type*)(allocator)->alloc(sizeof(type) * (count))
#define AllocatorFree(allocator, data) (allocator)->free(data)

struct Allocator {
    virtual void* alloc(u64 size)              = 0;
    virtual void* realloc(void* ptr, u64 size) = 0;
    virtual void  free(void* ptr) {};
    virtual void  clear() {};
};