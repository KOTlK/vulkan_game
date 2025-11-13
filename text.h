#pragma once

#include "basic.h"
#include <stdio.h>

bool read_entire_file_binary(const char* path, char** text, u32* size, Allocator* allocator) {
    FILE* file = fopen(path, "rb");

    if (!file) {
        printf("Cannot open file %s", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);

    if (size == 0) {
        fclose(file);
        return false;
    }

    fseek(file, 0, SEEK_SET);

    *text = AllocatorAlloc(char, allocator, *size);

    if (!(*text)) {
        fclose(file);
        return false;
    }

    fread(*text, 1, *size, file);
    fclose(file);

    return true;
}