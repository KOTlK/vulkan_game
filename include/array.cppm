module;

#include "basic.h"
#include "allocator.h"
#include "assert.h"

export module array;

#define ARRAY_TEMPLATE export template <typename T>

ARRAY_TEMPLATE
struct Array {
    T*         data;
    Allocator* allocator;
    u64        length;

    T* begin() { 
        Assert(data, "Cannot iterate uninitialized array, initialize it with array_make.");
        return data; 
    }

    T* end() {
        Assert(data, "Cannot iterate uninitialized array, initialize it with array_make.");
        return &data[length - 1]; 
    }

    const T* begin() const {
        Assert(data, "Cannot iterate uninitialized array, initialize it with array_make.");
        return data; 
    }

    const T* end() const { 
        Assert(data, "Cannot iterate uninitialized array, initialize it with array_make.");
        return &data[length - 1]; 
    }

    T& operator[](u64 i) {
        Assert(data, "Cannot index uninitialized array, initialize it with array_make.");
        Assert(i < length, "Index outside the bounds of the array");
        return data[i];
    }

    const T& operator[](u64 i) const {
        Assert(data, "Cannot index uninitialized array, initialize it with array_make.");
        Assert(i < length, "Index outside the bounds of the array");
        return data[i];
    }

    Array() : data(NULL), allocator(NULL), length(0){};
    ~Array() = default;
};


ARRAY_TEMPLATE
inline
void
array_make(Array<T>* array, u64 length, Allocator* allocator = Allocator_Persistent) {
    auto data = AllocatorAlloc(T, allocator, sizeof(T) * length);
    Assert(data, "Cannot allocate data for the array.");

    array->data      = data;
    array->length    = length;
    array->allocator = allocator;
}

ARRAY_TEMPLATE
inline
void
array_realloc(Array<T>* array, u64 length) {
    Assert(array->data, "Cannot realloc uninitialized array, initialize it with array_make.");
    Assert(length > array->length, "Cannot resize array with less size.");
    if (array->allocator == Allocator_Temp) {
        T* new_data = AllocatorAlloc(T, array->allocator, sizeof(T) * length);
        Assert(new_data, "Cannot allocate enough memory for new array");

        for (u64 i = 0; i < array->length; i++) {
            new_data[i] = array->data[i];
        }

        array->data = new_data;
    } else {
        array->data = (T*)array->allocator->realloc(array->data, sizeof(T) * length);
    }

    Assert(array->data, "Cannot realloc array");
    array->length = length;
}

ARRAY_TEMPLATE
inline
void
array_free(Array<T>* array) {
    Assert(array->data, "Cannot free uninitialized array, initialize it with array_make.");
    if (array->allocator == Allocator_Temp) return;

    AllocatorFree(array->allocator, array->data);
}

ARRAY_TEMPLATE
inline
T
array_get(Array<T>* array, u64 index) {
    Assert(array->data, "Cannot get item from uninitialized array, initialize it with array_make.");
    Assert(index < array->length - 1, "Index outside bounds of the array.");
    return array->data[index];
}

ARRAY_TEMPLATE
inline
T*
array_get_ptr(Array<T>* array, u64 index) {
    Assert(array->data, "Cannot get item pointer from uninitialized array, initialize it with array_make.");
    Assert(index < array->length - 1, "Index outside bounds of the array.");
    return &array->data[index];
}

ARRAY_TEMPLATE
inline
void
array_set(Array<T>* array, u64 index, T elem) {
    Assert(array->data, "Cannot set item to uninitialized array, initialize it with array_make.");
    Assert(index < array->length - 1, "Index outside bounds of the array.");
    array->data[index] = elem;
}

ARRAY_TEMPLATE
inline
void
array_clear(Array<T>* array) {
    Assert(array->data, "Cannot clear uninitialized array, initialize it with array_make.");
    for (u64 i = 0; i < array->length; i++) {
        array->data[i] = {0};
    }
}

ARRAY_TEMPLATE
inline
u32
array_index_of_ptr(Array<T>* array, T* elem) {
    Assert(array->data, "Cannot get index of pointer inside uninitialized array, initialize it with array_make.");
    u64 data_ptr = (u64)array->data;
    u64 elem_ptr = (u64)elem;

    u64 offset = elem_ptr - data_ptr;
    u64 size   = sizeof(T);

    return (u32)(offset / size);
}