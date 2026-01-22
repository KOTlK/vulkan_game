#pragma once

#include "basic.h"
#include "debug.h"
#include "assert.h"
#include <stdio.h>
#include <filesystem>

import text;

static inline bool   read_entire_file(const char* path, Allocator* allocator, String* str);
static inline bool   file_exists(const char* path);
static inline bool   file_exists(const String path);
static inline void   file_remove(const char* path);
static inline void   file_remove(const String path);

#ifdef FILE_IMPLEMENTATION

static inline bool read_entire_file(const char* path, Allocator* allocator, String* str) {
    FILE* file = NULL;
    errno_t err = fopen_s(&file, path, "rb");

    if (!file) {
        Errf("Cannot open file %s. Error: %d", path, err);
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

static inline bool file_exists(const char* path) {
    bool exist = std::filesystem::exists(path);

    return exist;
}

static inline bool file_exists(const String path) {
    return file_exists(path.text);
}

static inline void file_remove(const char* path) {
    Assertf(file_exists(path), "Cannot remove file %s. File does not exist.", path);

    bool removed = std::filesystem::remove(path);

    Assertf(removed, "Cannot remove file %s", path);
}

static inline void file_remove(const String path) {
    file_remove(path.text);
}
#endif