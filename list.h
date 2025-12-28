#pragma once

#include "assert.h"
#include "basic.h"
#include "allocator.h"
#include "memory.h"

#define LIST_DEFAULT_LENGTH 256
#define LIST_REALLOC_STEP 128

#define LIST_TEMPLATE template <typename T>

LIST_TEMPLATE
struct List {
    T*         data;
    u32        count;
    u32        length;
    Allocator* allocator;

    T* begin() {
        Assert(data, "Cannot iterate uninitialized list, use list_make to initialize it.");
        return data; 
    }

    T* end() { 
        Assert(data, "Cannot iterate uninitialized list, use list_make to initialize it.");
        return &data[count]; 
    }

    const T* begin() const { 
        Assert(data, "Cannot iterate uninitialized list, use list_make to initialize it.");
        return data; 
    }

    const T* end() const { 
        Assert(data, "Cannot iterate uninitialized list, use list_make to initialize it.");
        return &data[count]; 
    }

    T& operator[](u32 i) {
        Assert(data, "Cannot lookup uninitialized list, use list_make to initialize it.");
        Assert(i < count, "Index outside the bounds of the list");
        return data[i];
    }

    const T& operator[](u32 i) const {
        Assert(data, "Cannot lookup uninitialized list, use list_make to initialize it.");
        Assert(i < count, "Index outside the bounds of the list");
        return data[i];
    }

    List() : data(NULL), count(0), length(0), allocator(NULL){};
    ~List() = default;
};

LIST_TEMPLATE
static inline
void
list_make(List<T>* list, u32 length = LIST_DEFAULT_LENGTH, Allocator* allocator = Allocator_Persistent);

LIST_TEMPLATE
static inline
List<T>
list_make(u32 length = LIST_DEFAULT_LENGTH, Allocator* allocator = Allocator_Persistent);

LIST_TEMPLATE
static inline
void
list_realloc(List<T> *list, u32 length);

LIST_TEMPLATE
static inline
void
list_free(List<T> *list);

LIST_TEMPLATE
static inline
u32
list_append(List<T> *list, T element);

LIST_TEMPLATE
static inline
T*
list_append_empty(List<T> *list, u32* ret_index = NULL);

LIST_TEMPLATE
static inline
void
list_remove(List<T> *list, T element);

LIST_TEMPLATE
static inline
void
list_remove_swap_back(List<T> *list, T element);

LIST_TEMPLATE
static inline
void
list_remove_at(List<T> *list, u32 index);

LIST_TEMPLATE
static inline
void
list_remove_at_swap_back(List<T> *list, u32 index);

LIST_TEMPLATE
static inline
void
list_set(List<T> *list, u32 index, T element);

LIST_TEMPLATE
static inline
T
list_get(List<T> *list, u32 index);

LIST_TEMPLATE
static inline
T*
list_get_ptr(List<T> *list, u32 index);

LIST_TEMPLATE
static inline
void
list_quick_sort(List<T> *list);

LIST_TEMPLATE
static inline
void
list_flush(List<T> *list);

LIST_TEMPLATE
static inline
bool
list_contains(List<T> *list, T elem);

template <typename T, typename Predicate>
static inline
bool
list_contains(List<T> *list, Predicate descr, u32* index);

LIST_TEMPLATE
static inline
bool
list_find(List<T> *list, T elem, u32* index);

template <typename T, typename Predicate>
static inline
bool
list_find_by_descr(List<T> *list, Predicate descr, T* elem);

LIST_TEMPLATE
static inline
void
list_clear(List<T> *list);

// get the index of item, the pointer points to
LIST_TEMPLATE
static inline
u32
list_index_of_ptr(List<T>* list, T* elem);

// Implementation
LIST_TEMPLATE
static inline
void
list_make(List<T>* list, u32 length, Allocator* allocator) {
    auto data = AllocatorAlloc(T, allocator, sizeof(T) * length);
    Assert(data, "Cannot allocate list data.");

    list->data      = data;
    list->count     = 0;
    list->length    = length;
    list->allocator = allocator;
}

LIST_TEMPLATE
static inline
List<T>
list_make(u32 length, Allocator* allocator) {
    List<T> list{};

    auto data = AllocatorAlloc(T, allocator, sizeof(T) * length);
    Assert(data, "Cannot allocate list data.");

    list.data      = data;
    list.count     = 0;
    list.length    = length;
    list.allocator = allocator;

    return list;
}

LIST_TEMPLATE
static inline
void
list_realloc(List<T> *list, u32 length) {
    Assert(list->data, "Cannot realloc uninitialized list, use list_make to initialize it.");
    Assert(length > list->length, "Cannot resize list with less size.");

    if (list->allocator == Allocator_Temp) {
        T* new_data = AllocatorAlloc(T, list->allocator, sizeof(T) * length);
        Assert(new_data, "Cannot allocate enough memory for new list");

        for (u32 i = 0; i < list->count; i++) {
            new_data[i] = list->data[i];
        }

        list->data = new_data;
    } else {
        list->data = (T*)list->allocator->realloc(list->data, sizeof(T) * length);
    }

    Assert(list->data, "Cannot resize the list.");
    list->length = length;
}

LIST_TEMPLATE
static inline
void
list_free(List<T> *list) {
    Assert(list->data, "Cannot free uninitialized list, use list_make to initialize it.");
    // nothing to free if using Allocator_Temp
    if (list->allocator == Allocator_Temp) return;

    AllocatorFree(list->allocator, list->data);
    AllocatorFree(list->allocator, list);
}

LIST_TEMPLATE
static inline
u32
list_append(List<T> *list, T element) {
    Assert(list->data, "Cannot append to uninitialized list, use list_make to initialize it.");

    if (list->count >= list->length) {
        list_realloc(list, list->length + 1 + LIST_REALLOC_STEP);
    }
    
    u32 index = list->count;

    list->data[index]  = element;
    list->count       += 1;

    return index;
}

LIST_TEMPLATE
static inline
T*
list_append_empty(List<T> *list, u32* ret_index) {
    Assert(list->data, "Cannot append to uninitialized list, use list_make to initialize it.");

    if (list->count >= list->length) {
        list_realloc(list, list->length + 1 + LIST_REALLOC_STEP);
    }
    
    u32 index = list->count;

    T element{};

    list->data[index]  = element;
    list->count       += 1;

    if (ret_index != NULL)
        *ret_index = index;

    return &list->data[index];
}

LIST_TEMPLATE
static inline
void
list_remove(List<T> *list, T element) {
    Assert(list->data, "Cannot remove from uninitialized list, use list_make to initialize it.");

    u32 i = 0;
    for(; i < list->count; i++) {
        if (list->data[i] == element) {
            list->count--;
            break;
        }
    }

    for (; i < list->count; i++) {
        list->data[i] = list->data[i + 1];
    }
}

LIST_TEMPLATE
static inline
void
list_remove_swap_back(List<T> *list, T element) {
    Assert(list->data, "Cannot remove from uninitialized list, use list_make to initialize it.");

    for(u32 i = 0; i < list->count; i++) {
        if (list->data[i] == element) {
            list->data[i] = list->data[--list->count];
            break;
        }
    }
}

LIST_TEMPLATE
static inline
void
list_remove_at(List<T> *list, u32 index) {
    Assert(list->data, "Cannot remove from uninitialized list, use list_make to initialize it.");

    Assert(index < list->count, "Index outside the bounds of the list");
    list->count--;
    for (u32 i = index; i < list->count; i++) {
        list->data[i] = list->data[i + 1];
    }
}

LIST_TEMPLATE
static inline
void
list_remove_at_swap_back(List<T> *list, u32 index) {
    Assert(list->data, "Cannot remove from uninitialized list, use list_make to initialize it.");

    Assert(index < list->count, "Index outside the bounds of the list");
    list->data[index] = list->data[--list->count];
}

LIST_TEMPLATE
static inline
void
list_set(List<T> *list, u32 index, T element) {
    Assert(list->data, "Cannot set data in uninitialized list, use list_make to initialize it.");
    Assert(index < list->count, "Index outside the bounds of the list");
    list->data[index] = element;
}

LIST_TEMPLATE
static inline
T
list_get(List<T> *list, u32 index) {
    Assert(list->data, "Cannot get data from uninitialized list, use list_make to initialize it.");
    Assert(index < list->count, "Index outside the bounds of the list");
    return list->data[index];
}

LIST_TEMPLATE
static inline
T*
list_get_ptr(List<T> *list, u32 index) {
    Assert(list->data, "Cannot get data from uninitialized list, use list_make to initialize it.");
    Assert(index < list->count, "Index outside the bounds of the list");
    return &list->data[index];
}

LIST_TEMPLATE
static inline
void
swap(T *a, T *b) {
    T temp = *a;
    *a = *b;
    *b = temp;
}

LIST_TEMPLATE
static inline
void
quick_sort(T *arr, u32 low, u32 high) {
    if (low < high) {
        auto pivot = arr[high];
        auto i     = low - 1;

        for (u32 j = low; j < high; j++) {
            if (arr[j] < pivot) {
                i += 1;
                swap(&arr[i], &arr[j]);
            }
        }

        swap(&arr[i + 1], &arr[high]);
        pivot = i + 1;

        quick_sort(arr, low, pivot - 1);
        quick_sort(arr, pivot + 1, high);
    }
}

LIST_TEMPLATE
static inline
void
list_quick_sort(List<T> *list) {
    Assert(list->data, "Cannot sort uninitialized list, use list_make to initialize it.");
    quick_sort(list->data, 0, list->count);
}

LIST_TEMPLATE
static inline
void
list_flush(List<T> *list) {
    Assert(list->data, "Cannot flush uninitialized list, use list_make to initialize it.");
    list->count = 0;
}

LIST_TEMPLATE
static inline
bool
list_contains(List<T> *list, T elem) {
    Assert(list->data, "Cannot search in uninitialized list, use list_make to initialize it.");
    for (u32 i = 0; i < list->count; i++) {
        if (list->data[i] == elem) return true;
    }
    return false;
}

LIST_TEMPLATE
static inline
bool
list_find(List<T> *list, T elem, u32* index) {
    Assert(list->data, "Cannot search in uninitialized list, use list_make to initialize it.");
    for (u32 i = 0; i < list->count; i++) {
        if (list->data[i] == elem) {
            *index = i;
            return true;
        }
    }
    return false;
}

// Predicate should match signature:
// bool (*name)(T)
template <typename T, typename Predicate>
static inline
bool
list_find_by_descr(List<T> *list, Predicate descr, T* elem) {
    for (u32 i = 0; i < list->count; i++) {
        if (descr(list->data[i])) {
            *elem = list->data[i];
            return true;
        }
    }
    return false;
}

// Predicate should match signature:
// bool (*name)(T)
template <typename T, typename Predicate>
static inline
bool
list_contains(List<T> *list, Predicate descr, u32* index) {
    Assert(list->data, "Cannot search in uninitialized list, use list_make to initialize it.");
    for (u32 i = 0; i < list->count; i++) {
        if (descr(list->data[i])) {
            *index = i;
            return true;
        }
    }
    return false;
}

LIST_TEMPLATE
static inline
void
list_clear(List<T> *list) {
    Assert(list->data, "Cannot clear uninitialized list, use list_make to initialize it.");
    list->count = 0;

    memset(list->data, 0, sizeof(T) * list->length);
}

LIST_TEMPLATE
static inline
u32
list_index_of_ptr(List<T>* list, T* elem) {
    Assert(list->data, "Cannot get index of pointer inside uninitialized list, use list_make to initialize it.");
    // u64 data_ptr = (u64)list->data;
    // u64 elem_ptr = (u64)elem;

    // u64 offset = elem_ptr - data_ptr;
    u64 offset = elem - list->data;
    u64 size   = sizeof(T);

    return (u32)(offset / size);
}