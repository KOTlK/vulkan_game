#pragma once

#include "basic.h"
#include <string.h>
#include "debug.h"
#include "assert.h"
#include <stdio.h>

struct String {
    Allocator* allocator;
    char*      text;
    u32        length;
};

static inline String string_make_empty(u32 len, Allocator* allocator = Allocator_Persistent);
static inline String string_make(const char* text, Allocator* allocator = Allocator_Persistent);
static inline void   string_free(String* str);

static inline bool   read_entire_file(const char* path, Allocator* allocator, String* str);

#ifdef TEXT_IMPLEMENTATION

static inline String string_make_empty(u32 len, Allocator* allocator) {
    String str = {
        .allocator = allocator,
        .text      = AllocatorAlloc(char, allocator, len),
        .length    = len
    };
    
    Assertf(str.text, "Cannot allocate string with length %u\n", len);

    return str;
}

static inline String string_make(const char* text, Allocator* allocator) {
    u32 len = strlen(text);

    String str = {
        .allocator = allocator,
        .text      = AllocatorAlloc(char, allocator, len),
        .length    = len
    };

    strcpy(str.text, text);

    return str;
}

static inline void   string_free(String* str) {
    AllocatorFree(str->allocator, str->text);
}

static inline bool read_entire_file(const char* path, Allocator* allocator, String* str) {
    FILE* file = fopen(path, "rb");

    if (!file) {
        Errf("Cannot open file %s", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    str->length = ftell(file);

    if (str->length == 0) {
        fclose(file);
        return false;
    }

    fseek(file, 0, SEEK_SET);

    str->text = AllocatorAlloc(char, allocator, str->length);

    if (!str->text) {
        fclose(file);
        return false;
    }

    fread(str->text, 1, str->length, file);
    fclose(file);
    str->allocator = allocator;

    return true;
}

#endif // TEXT_IMPLEMENTATION