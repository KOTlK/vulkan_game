module;

#include "basic.h"
#include "memory.h"
#include "assert.h"

import queue;

export module rlist;

#define RLIST_TEMPLATE export template <typename T>
#define RLIST_REALLOC_STEP 256

RLIST_TEMPLATE
struct ReliableList {
    Allocator*  allocator;
    T*          data;
    Queue<u32>  free;
    u32         count;
    u32         length;
};


RLIST_TEMPLATE
inline void rlist_make(ReliableList<T>* list, u32 length, Allocator* allocator) {
    list->allocator = allocator;
    list->data      = AllocatorAlloc(T, allocator, sizeof(T) * length);
    list->count     = 0;
    list->length    = length;

    queue_make(&list->free, length, allocator);

    Assertf(list->data, "Cannot allocate %llu bytes of data for rlist.", sizeof(T) * length);
}

RLIST_TEMPLATE
inline void rlist_free(ReliableList<T>* list) {
    Assert(list, "Cannot free non existing rlist");
    if (list->allocator != Allocator_Temp) {
        AllocatorFree(list->allocator, list->data);
        queue_free(&list->free);
    }
}

RLIST_TEMPLATE
inline void rlist_realloc(ReliableList<T>* list, u32 len) {
    Assert(list, "Cannot realloc non existing rlist");

    T* data;

    if (list->allocator == Allocator_Temp) {
        data = AllocatorAlloc(T, list->allocator, len);

        memcpy(data, list->data, sizeof(T) * list->length);
    } else {
        data = (T*)list->allocator->realloc(list->data, sizeof(T) * len);
    }

    Assertf(data, "Cannot reallocate %llu bytes of data for rlist.", sizeof(T) * len);
    list->data   = data;
    list->length = len;
}

RLIST_TEMPLATE
inline u32 rlist_append(ReliableList<T>* list, T item) {
    Assert(list, "Cannot append into non existing rlist");
    u32 index = 0;
    if (list->free.count > 0) {
        index = queue_dequeue(&list->free);
    } else {
        index = list->count++;

        if (list->count >= list->length) {
            rlist_realloc(list, list->length + RLIST_REALLOC_STEP);
        }
    }

    list->data[index] = item;

    return index;
}

RLIST_TEMPLATE
inline T rlist_get(ReliableList<T>* list, u32 index) {
    Assert(list, "Cannot get item from non existing rlist");
    return list->data[index];
}

RLIST_TEMPLATE
inline void rlist_remove(ReliableList<T>* list, u32 index) {
    Assert(list, "Cannot remove item from non existing rlist");

    list->data[index] = {};
    queue_enqueue(&list->free, index);
}