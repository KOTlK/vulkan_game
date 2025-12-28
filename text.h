#pragma once

#include "basic.h"
#include <string.h>
#include "debug.h"
#include "assert.h"
#include <stdio.h>

#include "hash_functions.h"

struct String {
    Allocator* allocator;
    char*      text;
    u32        length;

    String() = default;
    String(const char* str);
};

inline String::String(const char* str) {
    allocator = Allocator_Persistent;
    length    = strlen(str);
    text      = AllocatorAlloc(char, allocator, sizeof(char) * (length + 1));
    
    strcpy(text, str);
    text[length] = '\0';
}

static inline String string_make_empty(u32 len, Allocator* allocator = Allocator_Persistent);
static inline String string_make(const char* text, Allocator* allocator = Allocator_Persistent);
static inline void   string_free(String* str);

static inline bool   read_entire_file(const char* path, Allocator* allocator, String* str);

static inline u64 get_hash(String string) {
    return get_hash(string.text);
}

#ifdef TEXT_IMPLEMENTATION

static inline String string_make_empty(u32 len, Allocator* allocator) {
    String str;

    str.allocator = allocator;
    str.text      = AllocatorAlloc(char, allocator, len + 1);
    str.length    = len;
    
    Assertf(str.text, "Cannot allocate string with length %u\n", len);

    memset(str.text, 0, str.length + 1);

    return str;
}

static inline String string_make(const char* text, Allocator* allocator) {
    u32 len = strlen(text);

    String str;

    str.allocator = allocator;
    str.text      = AllocatorAlloc(char, allocator, len + 1);
    str.length    = len;

    strcpy(str.text, text);

    str.text[str.length] = '\0';

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