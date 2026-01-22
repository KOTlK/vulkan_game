module;

#include "basic.h"
#include <string.h>
#include "debug.h"
#include "assert.h"

#include "hash_functions.h"

export module text;

#define STRING_BUILDER_INITIAL_LENGTH 512
#define STRING_BUILDER_REALLOC_STEP 256

export struct String {
    Allocator* allocator;
    char*      text;
    u32        length;

    String() = default;
    String(const char* str, Allocator* allocator = Allocator_Persistent);

    char* begin() {
        Assert(text, "Cannot iterate uninitialized string, use string_make or constructor to initialize it.");
        return text; 
    }

    char* end() { 
        Assert(text, "Cannot iterate uninitialized string, use string_make or constructor to initialize it.");
        return &text[length]; 
    }

    const char* begin() const { 
        Assert(text, "Cannot iterate uninitialized string, use string_make or constructor to initialize it.");
        return text; 
    }

    const char* end() const { 
        Assert(text, "Cannot iterate uninitialized string, use string_make or constructor to initialize it.");
        return &text[length]; 
    }

    inline char& operator[](u32 i) {
        Assert(text, "Cannot lookup uninitialized string, use string_make or constructor to initialize it.");
        if (i >= length) {
            Log("HEllo");
        }
        Assert(i < length, "Index outside the bounds of the string");
        return text[i];
    }

    inline const char& operator[](u32 i) const {
        Assert(text, "Cannot lookup uninitialized string, use string_make or constructor to initialize it.");
        Assert(i < length, "Index outside the bounds of the string");
        return text[i];
    }
};

export struct StringBuilder {
    Allocator* allocator;
    char*      text;
    u32        length;
    u32        count;
};

inline String::String(const char* str, Allocator* allocator) {
    this->allocator = allocator;
    length          = strlen(str);
    text            = AllocatorAlloc(char, allocator, sizeof(char) * (length + 1));

    for (u32 i = 0; i < length; i++) {
        text[i] = str[i];
    }
    
    // strcpy_s(text, length, str);
    text[length] = '\0';
}

// export inline bool operator==(String lhs, String rhs);
// export inline bool operator==(String lhs, const char* rhs);
// export inline bool operator!=(String lhs, String rhs);
// export inline bool operator!=(String lhs, const char* rhs);

// export inline String  string_make_empty(u32 len, Allocator* allocator = Allocator_Persistent);
// export inline String* string_make_empty_ptr(u32 len, Allocator* text_allocator = Allocator_Persistent, Allocator* ptr_allocator = Allocator_Persistent);
// export inline String  string_make(const char* text, Allocator* allocator = Allocator_Persistent);
// export inline String* string_make_ptr(const char* text, Allocator* text_allocator = Allocator_Persistent, Allocator* ptr_allocator = Allocator_Persistent);
// export inline void    string_free(String* str);

// export inline bool   string_contains(String* str, const char c);
// export inline bool   string_ends_with(String* str, const char c);
// export inline String string_substring(String* str, u32 start, u32 end, Allocator* allocator = Allocator_Persistent);
// export inline bool   string_equals(String lhs, String rhs);

// export inline StringBuilder sb_make(u32 len = STRING_BUILDER_INITIAL_LENGTH, Allocator* allocator = Allocator_Persistent);
// export inline void    sb_realloc(StringBuilder* sb, const u32 len);
// export inline void    sb_free(StringBuilder* sb);
// export inline void    sb_append(StringBuilder* sb, const char c);
// export inline void    sb_append(StringBuilder* sb, const String str);
// export inline void    sb_append(StringBuilder* sb, const char* str);
// export inline void    sb_append_line(StringBuilder* sb);
// export inline void    sb_append_line(StringBuilder* sb, const String str);
// export inline void    sb_append_line(StringBuilder* sb, const char* str);
// export inline void    sb_clear(StringBuilder* sb);
// export inline String  sb_to_string(StringBuilder* sb, Allocator* allocator = Allocator_Persistent);
// export inline String* sb_to_string_ptr(StringBuilder* sb, Allocator* text_allocator = Allocator_Persistent, Allocator* ptr_allocator = Allocator_Persistent);
// export inline char*   sb_to_cstring(StringBuilder* sb, Allocator* allocator = Allocator_Persistent);

export inline u64 get_hash(String string) {
    return get_hash(string.text);
}


export inline bool operator==(String lhs, String rhs) {
    if (lhs.length != rhs.length) return false;

    for (u32 i = 0; i < lhs.length; i++) {
        if (lhs[i] != rhs[i]) return false;
    }

    return true;
}

export inline bool operator==(String lhs, const char* rhs) {
    u32 len = strlen(rhs);

    if (lhs.length != len) return false;

    for (u32 i = 0; i < len; i++) {
        if (lhs[i] != rhs[i]) return false;
    }

    return true;
}

export inline bool operator!=(String lhs, String rhs) {
    return !(lhs == rhs);
}

export inline bool operator!=(String lhs, const char* rhs) {
    return !(lhs == rhs);
}

export inline String string_make_empty(u32 len, Allocator* allocator = Allocator_Persistent) {
    String str{};

    str.allocator = allocator;
    str.text      = AllocatorAlloc(char, allocator, len + 1);
    str.length    = len;
    
    Assertf(str.text, "Cannot allocate string with length %u\n", len);

    memset(str.text, 0, str.length + 1);

    return str;
}

export inline String* string_make_empty_ptr(u32 len, Allocator* text_allocator = Allocator_Persistent, Allocator* ptr_allocator = Allocator_Persistent) {
    String* str = AllocatorAlloc(String, ptr_allocator, sizeof(String));

    str->allocator = text_allocator;
    str->text      = AllocatorAlloc(char, text_allocator, len + 1);
    str->length    = len;
    
    Assertf(str->text, "Cannot allocate string with length %u\n", len);

    memset(str->text, 0, str->length + 1);

    return str;
}

export inline String string_make(const char* text, Allocator* allocator = Allocator_Persistent) {
    u32 len = strlen(text);

    String str{};

    str.allocator = allocator;
    str.text      = AllocatorAlloc(char, allocator, len + 1);
    str.length    = len;

    Assert(str.text, "Cannot allocate memory for text");

    for (u32 i = 0; i < len; i++) {
        str[i] = text[i];
    }

    // strcpy_s(str.text, len, text);

    str.text[str.length] = '\0';

    return str;
}

export inline String* string_make_ptr(const char* text, Allocator* text_allocator = Allocator_Persistent, Allocator* ptr_allocator = Allocator_Persistent) {
    String* str = AllocatorAlloc(String, ptr_allocator, sizeof(String));
    
    u32 len = strlen(text);
    str->allocator = text_allocator;
    str->text      = AllocatorAlloc(char, text_allocator, len + 1);
    str->length    = len;

    Assert(str->text, "Cannot allocate memory for text");

    for (u32 i = 0; i < len; i++) {
        str->text[i] = text[i];
    }

    // strcpy_s(str->text, len, text);

    str->text[str->length] = '\0';

    return str;
}

export inline void   string_free(String* str) {
    AllocatorFree(str->allocator, str->text);
}

export inline bool string_contains(String* str, const char c) {
    Assert(str->text, "Cannot search in uninitialized string.");

    for(u32 i = 0; i < str->length; i++) {
        if (str->text[i] == c) return true;
    }

    return false;
}

export inline bool string_ends_with(String* str, const char c) {
    Assert(str->text, "Cannot search in uninitialized string.");

    return (str->text[str->length - 1] == c);
}

export inline String string_substring(String* str, u32 start, u32 end, Allocator* allocator) {
    Assert(str->text, "Cannot substring from uninitialized string.");
    Assertf(start < end, "Substring start index(%d) should be less then the end index(%d).", start, end);
    Assertf(end < str->length, "Substring end(%d) should be less then the input string length(%d).", end, str->length);
    String out{};
    u32 len = end - start;

    out = string_make_empty(len, allocator);

    for (u32 i = 0; i < len; i++) {
        out[i] = str->text[start + i];
    }

    return out;
}

export inline bool string_equals(String lhs, String rhs) {
    if (lhs.length != rhs.length) return false;

    for (u32 i = 0; i < lhs.length; i++) {
        if (lhs[i] != rhs[i]) return false;
    }

    return true;
}

export inline StringBuilder sb_make(u32 len = STRING_BUILDER_INITIAL_LENGTH, Allocator* allocator = Allocator_Persistent) {
    StringBuilder sb;

    sb.allocator = allocator;
    sb.text      = AllocatorAlloc(char, allocator, sizeof(char) * len);
    sb.length    = len;
    sb.count     = 0;

    Assertf(sb.text, "Cannot allocate %d bytes for string builder", len);

    return sb;
}

export inline void sb_realloc(StringBuilder* sb, const u32 len) {
    if (sb->allocator == Allocator_Temp) {
        sb->text = AllocatorAlloc(char, sb->allocator, sizeof(char) * len);
    } else {
        sb->text = (char*)sb->allocator->realloc(sb->text, sizeof(char) * len);
    }

    Assertf(sb->text, "Cannot reallocate %dB for string builder.", len);

    sb->length = len;
}

export inline void sb_free(StringBuilder* sb) {
    AllocatorFree(sb->allocator, sb->text);
}

export inline void sb_append(StringBuilder* sb, const char c) {
    if (sb->count >= sb->length) {
        sb_realloc(sb, sb->count + STRING_BUILDER_REALLOC_STEP);
    }

    sb->text[sb->count++] = c;
}

export inline void sb_append(StringBuilder* sb, const String str) {
    if (sb->count + str.length >= sb->length) {
        sb_realloc(sb, sb->count + str.length + STRING_BUILDER_REALLOC_STEP);
    }

    for (u32 i = 0; i < str.length; i++) {
        sb->text[sb->count + i] = str.text[i];
    }

    sb->count += str.length;
}

export inline void sb_append(StringBuilder* sb, const char* str) {
    u32 len = strlen(str);
    if (sb->count + len >= sb->length) {
        sb_realloc(sb, sb->count + len + STRING_BUILDER_REALLOC_STEP);
    }

    for (u32 i = 0; i < len; i++) {
        sb->text[sb->count + i] = str[i];
    }

    sb->count += len;
}

export inline void sb_append_line(StringBuilder* sb) {
    sb_append(sb, '\n');
}

export inline void sb_append_line(StringBuilder* sb, const String str) {
    sb_append(sb, str);
    sb_append(sb, '\n');
}

export inline void sb_append_line(StringBuilder* sb, const char* str) {
    sb_append(sb, str);
    sb_append(sb, '\n');
}

export inline void sb_clear(StringBuilder* sb) {
    sb->count = 0;
}

export inline String sb_to_string(StringBuilder* sb, Allocator* allocator = Allocator_Persistent) {
    String str = string_make_empty(sb->count, allocator);

    for (u32 i = 0; i < sb->count; i++) {
        str.text[i] = sb->text[i];
    }

    return str;
}

export inline String* sb_to_string_ptr(StringBuilder* sb, Allocator* text_allocator = Allocator_Persistent, Allocator* ptr_allocator = Allocator_Persistent) {
    String* str = string_make_empty_ptr(sb->count, text_allocator, ptr_allocator);

    for (u32 i = 0; i < sb->count; i++) {
        str->text[i] = sb->text[i];
    }

    return str;
}

export inline char* sb_to_cstring(StringBuilder* sb, Allocator* allocator = Allocator_Persistent) {
    char* str = AllocatorAlloc(char, allocator, sizeof(char) * sb->count);

    for (u32 i = 0; i < sb->count; i++) {
        str[i] = sb->text[i];
    }

    return str;
}