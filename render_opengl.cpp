#define TEXT_IMPLEMENTATION
#include "glass.h"
#include "render.h"
#include "debug.h"
#include "glad/glad.h"
#include "text.h"
#include "list.h"
#include "hash_table.h"
#include <cstddef>
#include "Vector3.h"
#include "Matrix4.h"

#ifdef GLASS_SDL
#include "glass_sdl.h"
#endif

struct CameraData {
    Matrix4 view;
    Matrix4 proj;
    Matrix4 view_proj;
    Vector3 position;
};

struct TimeData {
    float dt;
    float time;
    float sin_time;
    float cos_time;
};

struct ShapeCache {
    u32 vao;
    u32 vbo;
    u32 ebo;
};

struct Shader {
    String name;
    u32    gl_shader;
};

struct Material {
    Shader* shader;
};

struct RenderContext {
#ifdef GLASS_SDL
    SDL_GLContext sdl_context;
#endif
    List<Shader>                    shaders;
    List<Material>                  materials;
    HashTable<Shape2D*, ShapeCache> shape_cache;
};

Material* Active_Material;
Shape2D   Shape;

static inline void use_shader(Shader* shader);
static inline ShapeCache get_shape_cache(Shape2D* shape);

static RenderContext Render_Context{};

RenderError render_init(Context* ctx) {
    list_make(&Render_Context.shaders);
    list_make(&Render_Context.materials);
    table_make(&Render_Context.shape_cache);

    int glad_version = 0;
#ifdef GLASS_SDL
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    Render_Context.sdl_context  = SDL_GL_CreateContext(ctx->wnd->window);
    auto make_context = SDL_GL_MakeCurrent(ctx->wnd->window, Render_Context.sdl_context);

    if (!make_context) {
        Err("Cannot setup gl context");
        return RENDER_INTERNAL_ERROR;
    }

    glad_version = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
#endif

    if (glad_version == 0) {
        Errf("Cannot load glad.");
        return RENDER_INTERNAL_ERROR;
    }

    Logf("Glad version: %i.", glad_version);

    bool read = false;

    String vert_text;
    String frag_text;

    read = read_entire_file("shaders/vert.shader", Allocator_Temp, &vert_text);

    if (!read) {
        Err("Cannot read vertex shader.");
    }

    read = read_entire_file("shaders/frag.shader", Allocator_Temp, &frag_text);

    if (!read) {
        Err("Cannot read fragment shader.");
    }

    RenderError err = RENDER_OK;

    Shader* shader = shader_make(&vert_text, &frag_text, &err);
    Active_Material = material_make(shader);

    if (err) {
        Errf("Cannot create active shader. %d", err);
        return RENDER_INTERNAL_ERROR;
    }

    Vertex vertices[] = {
        {{{   0.5f,  -0.5f, 0.0f }}, { 255, 0,   0,   255 }},
        {{{   0.5f,   0.5f, 0.0f }}, { 0,   255, 0,   255 }},
        {{{  -0.5f,   0.5f, 0.0f }}, { 0,   0,   255, 255 }},
        {{{  -0.5f,  -0.5f, 0.0f }}, { 255, 255, 0, 255 }},
    };

    u16 indices[] = {
        0, 1, 3,
        1, 2, 3
    };  

    shape2d_make(vertices, indices, sizeof(vertices) / sizeof(Vertex), sizeof(indices) / sizeof(u16), &Shape);

    return RENDER_OK;
}

void render_destroy() {
#ifdef GLASS_SDL
    SDL_GL_DestroyContext(Render_Context.sdl_context);
#endif
}

RenderError render_test() {
    glClearColor(0.3f, 0.3f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Matrix4 mat = matrix4_trs_2d(0, 0, 0, 1, 1);

    render_shape_2d(Active_Material, &Shape, mat);
    
    return RENDER_OK;
}

Shader* shader_make(String* vert_text, String* frag_text, RenderError* err) {
    Shader* shader = list_append_empty(&Render_Context.shaders);

    u32 vert = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vert, 1, &vert_text->text, (const GLint*)&vert_text->length);
    glCompileShader(vert);

    int  success;
    char log[512];
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vert, 512, NULL, log);
        Errf("Cannot compile vertex shader. %s", log);
        *err = RENDER_INTERNAL_ERROR;
        return NULL;
    }

    u32 frag = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(frag, 1, &frag_text->text, (const GLint*)&frag_text->length);
    glCompileShader(frag);

    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, log);
        Errf("Cannot compile fragment shader. %s", log);
        *err = RENDER_INTERNAL_ERROR;
        return NULL;
    }

    shader->gl_shader = glCreateProgram();

    glAttachShader(shader->gl_shader, vert);
    glAttachShader(shader->gl_shader, frag);
    glLinkProgram(shader->gl_shader);

    glGetProgramiv(shader->gl_shader, GL_LINK_STATUS, &success);

    if(!success) {
        glGetProgramInfoLog(shader->gl_shader, 512, NULL, log);
        Err(log);
        *err = RENDER_INTERNAL_ERROR;
        return NULL;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);

    return shader;
}

void shader_destroy(Shader* shader) {
    glDeleteProgram(shader->gl_shader);
}

Material* material_make(Shader* shader) {
    Material* mat = list_append_empty(&Render_Context.materials);

    mat->shader = shader;

    return mat;
}

void material_destroy(Material* mat) {
    
}

RenderError render_shape_2d(Material* mat, Shape2D* shape, Matrix4 trs) {
    use_shader(mat->shader);

    ShapeCache cache = get_shape_cache(shape);

    glBindVertexArray(cache.vao);

    glDrawElements(GL_TRIANGLES, shape->index_count, GL_UNSIGNED_SHORT, 0);

    return RENDER_OK;
}

static inline void use_shader(Shader* shader) {
    glUseProgram(shader->gl_shader);
}

static inline ShapeCache get_shape_cache(Shape2D* shape) {
    ShapeCache cache;

    if (table_try_get(&Render_Context.shape_cache, shape, &cache)) {
        return cache;
    }

    Log("Creating shape cache.");

    u32 vbo;
    u32 ebobo;
    u32 vao;

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebobo);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * shape->vertex_count, shape->vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebobo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * shape->index_count, shape->indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebobo);
    glBindVertexArray(0);

    cache = {
        .vao = vao,
        .vbo = vbo,
        .ebo = ebobo
    };

    table_add(&Render_Context.shape_cache, shape, cache);

    return cache;
}