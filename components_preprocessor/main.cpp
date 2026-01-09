#define TEXT_IMPLEMENTATION
#define FILE_IMPLEMENTATION
#include "basic.h"
#include <stdio.h>
#include "debug.h"
#include "text.h"
#include "list.h"
#include "file.h"

// #define PRINT_DEBUG

struct ComponentDeclaration {
    String type;
    u32    index;
};

static inline bool   is_letter(char c);
static inline bool   is_number(char c);
static inline bool   is_special(char c);
static inline String parse_word(String in, StringBuilder* sb, u32* i);
static inline u32    convert_to_multiple_64(u32 value);

int main(int argc, char** argv) {
    List<ComponentDeclaration> components = list_make<ComponentDeclaration>();
#ifdef PRINT_DEBUG
    Logf("argc: %d.", argc);

    for (u32 i = 0; i < argc; i++) {
        Logf("  %s.", argv[i]);
    }
#endif

    if (argc < 3) return 1;


    String input{};

    read_entire_file(argv[1], Allocator_Persistent, &input);

    StringBuilder sb      = sb_make();
    StringBuilder h_out   = sb_make();
    StringBuilder cpp_out = sb_make();

    u32 i = 0;
    u32 line = 1;
    u32 line_start = 1;
    u32 component_bit = 0;
    char buf[512];

    // .h file
    for (;;) {
        if (i >= input.length) break;

        auto c = input[i];

        if (c == '\n') {
            line++;
            line_start = i;
        }

        if (c == '#') {
            if (input[i+1] == '#') {
                sb_append(&h_out, '#');
                i++;
                continue;
            }

            if (i > 0 && input[i - 1] == '#') {
                sb_append(&h_out, '#');
                i++;
                continue;
            }

            i++;
            auto directive = parse_word(input, &sb, &i);
#ifdef PRINT_DEBUG
            Logf("Directive: %s.", directive.text);
#endif

            if (directive == "DECLARE_COMPONENT") {
                if (input[i] != '(') {
                    Logf("Unexpected token at %d:%d. Expected %c, got %c.", line, i - line_start, '(', input[i]);
                    break;
                }

                i++;

                auto type = parse_word(input, &sb, &i);

                ComponentDeclaration component = {
                    .type = type,
                    .index = component_bit,
                };

                if (input[i] != ')') {
                    Logf("Unexpected token at %d:%d. Expected %c, got %c.", line, i - line_start, ')', input[i]);
                    break;
                }

                i++;

#ifdef PRINT_DEBUG
                Logf("Component: %s", type.text);
#endif

                sprintf(buf, "extern ComponentTable %s_s;\n", type.text);

                sb_append(&h_out, buf);

                sprintf(buf, "extern u32 %s_bit;\n", type.text);

                sb_append(&h_out, buf);

                component_bit++;

                list_append(&components, component);

                continue;
            }

            sb_append(&h_out, '#');
            sb_append(&h_out, directive);
        } else {
            sb_append(&h_out, c);
            i++;
        }
    }

    sprintf(buf, "#define COMPONENTS_COUNT %d\n", component_bit);

    sb_append(&h_out, buf);

    u32 bit_count = convert_to_multiple_64(component_bit);

    sprintf(buf, "#define ARCHETYPE_BIT_COUNT %d\n", bit_count);

    sb_append(&h_out, buf);

    sprintf(buf, "typedef Bitmap<ARCHETYPE_BIT_COUNT> Archetype;\n");

    sb_append(&h_out, buf);

    auto o = sb_to_string(&h_out);

#ifdef PRINT_DEBUG
    printf("%s\n", o.text);
#endif

    if (file_exists(argv[2])) {
        file_remove(argv[2]);
    }

    FILE* out_f = NULL;

    auto err = fopen_s(&out_f, argv[2], "wb");

    Assertf(out_f, "Cannot create .h file for writing. Error: %d", err);

    fwrite(o.text, 1, o.length, out_f);

    fclose(out_f);

    // cpp file
    sb_append_line(&cpp_out, "#include \"component_system.h\"");
    sb_append_line(&cpp_out, "#include \"components.h\"");
    sb_append_line(&cpp_out);

    for (auto c : components) {
        sprintf(buf, "ComponentTable %s_s = component_table_make(sizeof(%s));", c.type.text, c.type.text);
        sb_append_line(&cpp_out, buf);

        sprintf(buf, "u32 %s_bit = %d;", c.type.text, c.index);
        sb_append_line(&cpp_out, buf);
        sb_append_line(&cpp_out);
    }

    sb_append_line(&cpp_out, "ComponentTable* All_Components[] = {");

    for (auto c : components) {
        sprintf(buf, "  &%s_s,", c.type.text);
        sb_append_line(&cpp_out, buf);
    }

    sb_append_line(&cpp_out, "};");

    sb_append_line(&cpp_out, "const char* Component_Name_By_Bit[] = {");

    for (auto c : components) {
        sprintf(buf, "  \"%s\",", c.type.text);
        sb_append_line(&cpp_out, buf);
    }

    sb_append_line(&cpp_out, "};");

    sb_append_line(&cpp_out);

    sb_append_line(&cpp_out, "static inline ComponentTable* get_component_table_by_bit(u32 bit) {");
    sb_append_line(&cpp_out, "  return All_Components[bit];");
    sb_append_line(&cpp_out, "}");

    auto cpp = sb_to_string(&cpp_out);

#ifdef PRINT_DEBUG
    printf("%s\n", cpp.text);
#endif

    if (file_exists(argv[3])) {
        file_remove(argv[3]);
    }

    FILE* out_cpp_f = NULL;

    err = fopen_s(&out_cpp_f, argv[3], "wb");

    Assertf(out_cpp_f, "Cannot create .cpp file for writing. Error: %d", err);

    fwrite(cpp.text, 1, cpp.length, out_cpp_f);

    fclose(out_cpp_f);

    return 0;
}

static inline bool is_letter(char c) {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

static inline bool is_number(char c) {
    return (c >= 48 && c <= 57);
}

static inline bool is_special(char c) {
    return ((c >= 33 &&  c <= 47) ||
            (c >= 58 &&  c <= 64) ||
            (c >= 91 &&  c <= 96) ||
            (c >= 123 && c <= 126));
}

static inline String parse_word(String in, StringBuilder* sb, u32* i) {
    sb_clear(sb);

    for (;;) {
        if (*i >= in.length) break;

        char c = in[*i];

        if (*i == in.length) {
            auto str = sb_to_string(sb);
            sb_clear(sb);
            return str;
        }

        if (c == ' ') break;
        if (c == '\n') break;
        if (c == '\r') break;
        if (c == '\t') break;
        if (is_special(c) && c != '_') break;

        sb_append(sb, c);
        (*i) += 1;
    }

    auto str = sb_to_string(sb);
    sb_clear(sb);

    return str;
}

static inline u32 convert_to_multiple_64(u32 value) {
    if (value == 0) {
        return 64;
    }
    
    u32 result = (value + 63) & ~63;
    
    return (result < 64) ? 64 : result;
}