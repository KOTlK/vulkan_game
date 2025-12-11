#pragma once

#include "types.h"
#include "allocator.h"

#ifdef ALLOCATOR_STD_CUSTOM_MALLOC
#else
    #include <malloc.h>

    #define Allocator_Std_Malloc(size)       ::malloc(size)
    #define Allocator_Std_Realloc(ptr, size) ::realloc(ptr, size)
    #define Allocator_Std_Free(ptr)          ::free(ptr)
#endif

#ifdef MEMORY_DEBUG
    #include <unordered_map>
#endif

struct AllocatorPersistent : public Allocator{
#ifdef MEMORY_DEBUG
    u64 allocated;
    std::unordered_map<void*, u64> sizes;
#endif
    AllocatorPersistent();
    ~AllocatorPersistent();
    void* alloc(u64 size) override;
    void* realloc(void* ptr, u64 size) override;
    void  free(void* ptr) override;
};

inline AllocatorPersistent::AllocatorPersistent() {
#ifdef MEMORY_DEBUG
    allocated = 0;
#endif
}

inline AllocatorPersistent::~AllocatorPersistent() {
}

inline void* AllocatorPersistent::alloc(u64 size) {
    void* data = Allocator_Std_Malloc(size);
    
#ifdef MEMORY_DEBUG
    allocated += size;
    sizes.insert({data, size});
#endif  
    return data;
}

inline void* AllocatorPersistent::realloc(void* ptr, u64 size) {
    void* data = Allocator_Std_Realloc(ptr, size);

#ifdef MEMORY_DEBUG
    u64 prev       = sizes[ptr];
    u64 diff       = size - prev;
        allocated += diff;
    if (data != ptr) {
        sizes.erase(ptr);
        sizes.insert({data, size});
    } else {
        sizes.insert_or_assign(data, size);
    }
#endif  
    return data;
}

inline void AllocatorPersistent::free(void* ptr) {
#ifdef MEMORY_DEBUG
    u64 size       = sizes[ptr];
        allocated -= size;
    sizes.erase(ptr);
#endif
    Allocator_Std_Free(ptr);
}