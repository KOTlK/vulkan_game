#pragma once

#include "types.h"
#include "allocator.h"
#include "assert.h"

#ifdef ARENA_CUSTOM_MALLOC
#else
    #include "malloc.h"
    #define Arena_Malloc(type, size)       (type*)malloc(size)
    #define Arena_Realloc(type, ptr, size) (type*)realloc(ptr, size)
    #define Arena_Free(ptr)                       free(ptr)
#endif

#define ARENA_BUCKET_SIZE 1024 * 1024 * 8

struct ArenaBucket {
    ArenaBucket* next;
    u8*          data;
    u64          capacity;
    u64          allocated;
};

struct Arena : public Allocator {
    ArenaBucket* start;
#ifdef MEMORY_DEBUG
    u64          total_capacity;
    u64          allocated;
    u64          buckets_count;
#endif
    Arena();
    ~Arena();
    void* alloc(u64 size) override;
    void* realloc(void* ptr, u64 size) override;
    void  clear() override;
};

static inline ArenaBucket* arena_bucket_make(u64 capacity);
static inline void         arena_bucket_free(ArenaBucket* bucket);
static inline void         arena_bucket_clear(ArenaBucket* bucket);
static inline ArenaBucket* arena_get_fit_bucket(Arena* arena, u64 size);

inline Arena::Arena() {
    start = arena_bucket_make(ARENA_BUCKET_SIZE);

#ifdef MEMORY_DEBUG
    total_capacity = ARENA_BUCKET_SIZE;
    allocated      = 0;
    buckets_count  = 1;
#endif
}

inline Arena::~Arena() {
    arena_bucket_free(start);
}

inline void* Arena::alloc(u64 size) {
    ArenaBucket* bucket = arena_get_fit_bucket(this, size);
    
    u64 offset = bucket->allocated;

    bucket->allocated += size;

#ifdef MEMORY_DEBUG
    allocated += size;
#endif
    
    return (void*)(bucket->data + offset);
}

inline void* Arena::realloc(void* ptr, u64 size) {
    ArenaBucket* bucket = arena_get_fit_bucket(this, size);
    
    u64 offset = bucket->allocated;

    bucket->allocated += size;

#ifdef MEMORY_DEBUG
    allocated += size;
#endif
    
    return (void*)(bucket->data + offset);
}

inline void Arena::clear() {
    arena_bucket_clear(start);
#ifdef MEMORY_DEBUG
    allocated = 0;
#endif
}

static inline ArenaBucket* arena_bucket_make(u64 capacity) {
    ArenaBucket* bucket = Arena_Malloc(ArenaBucket, sizeof(ArenaBucket));

    Assert(bucket, "Not enough memory for arena bucket");
    
    bucket->allocated = 0;
    bucket->capacity  = capacity;
    bucket->data      = Arena_Malloc(u8, capacity);
    bucket->next      = NULL;

    Assertf(bucket->data, "Not enough memory for arena data. Wanted capacity: %llu", capacity);

    return bucket;
}

static inline void arena_bucket_free(ArenaBucket* bucket) {
    if (bucket->next) {
        arena_bucket_free(bucket->next);
    }

    Arena_Free(bucket);
}

static inline void arena_bucket_clear(ArenaBucket* bucket) {
    bucket->allocated = 0;

    if (bucket->next) {
        arena_bucket_clear(bucket->next);
    }
}

static inline ArenaBucket* arena_get_fit_bucket(Arena* arena, u64 size) {
    ArenaBucket* start = arena->start;

    while((start->allocated + size) > start->capacity) {
        if (!start->next) {
            u64 capacity = size > ARENA_BUCKET_SIZE ? size : ARENA_BUCKET_SIZE;
            start->next = arena_bucket_make(capacity);

#ifdef MEMORY_DEBUG
            arena->buckets_count++;
            arena->total_capacity += size;
#endif

            Assertf(start->next, "Cannot allocate arena bucket with size %llu.", size);
        }
        
        start = start->next;
    }

    return start;
}