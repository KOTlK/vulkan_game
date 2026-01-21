#define GAME_MATH_IMPLEMENTATION
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
#include "Vector4.h"

#ifdef GLASS_SDL
#include "glass_sdl.h"
#endif

struct CameraData {
    Matrix4 v;
    Matrix4 p;
    Matrix4 vp;
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
    HashTable<String, GLint> uniforms;
};

struct RenderContext {
#ifdef GLASS_SDL
    SDL_GLContext sdl_context;
#endif
    List<Shader>                    shaders;
    List<Material>                  materials;
    HashTable<Shape2D*, ShapeCache> shape_cache;
};

// Material*  Active_Material;
// Shape2D    Shape;
CameraData Camera_Data;
Camera*    Active_Camera;
TimeData   Time_Data;
u32        Camera_UBO;
u32        Time_UBO;

static inline void use_shader(Shader* shader);
static inline ShapeCache get_shape_cache(Shape2D* shape);

static RenderContext Render_Context{};

RenderError render_init(Game_Context* ctx) {
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
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

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

    glGenBuffers(1, &Camera_UBO);
    glGenBuffers(1, &Time_UBO);

    return RENDER_OK;
}

void render_destroy() {
#ifdef GLASS_SDL
    SDL_GL_DestroyContext(Render_Context.sdl_context);
#endif
}

void clear_color_buffer(Vector4 color) {
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}

RenderError render_test() {
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

    mat->shader   = shader;
    mat->uniforms = table_make<String, GLint>();

    GLint uniform_count = 0;
    glGetProgramiv(shader->gl_shader, GL_ACTIVE_UNIFORMS, &uniform_count);

    for (GLint i = 0; i < uniform_count; ++i) {
        GLchar name[256];
        GLsizei length = 0;
        GLint   size   = 0;
        GLenum  type   = 0;

        glGetActiveUniform(shader->gl_shader, i,
                           sizeof(name),        
                           &length,             
                           &size,               
                           &type,               
                           name);


        GLint location = glGetUniformLocation(shader->gl_shader, name);
        Logf("Found uniform: %s, location: %d", name, location);

        table_add(&mat->uniforms, string_make(name), location);
    }

    return mat;
}

void material_destroy(Material* mat) {
    
}

void render_set_active_camera(Camera* cam) {
    Active_Camera = cam;
}

void render_set_camera_matrices(Matrix4 v, Matrix4 p, Vector3 pos) {
    Camera_Data.v        = v;
    Camera_Data.p        = p;
    Camera_Data.vp       = p * v;
    Camera_Data.position = pos;

    glBindBuffer(GL_UNIFORM_BUFFER, Camera_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), &Camera_Data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void render_set_time(float dt, float time) {
    Time_Data.dt       = dt;
    Time_Data.time     = time;
    Time_Data.sin_time = sin(time);
    Time_Data.cos_time = cos(time);

    glBindBuffer(GL_UNIFORM_BUFFER, Time_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(TimeData), &Time_Data, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

RenderError render_shape_2d(Material* mat, Shape2D* shape, Transform* transform) {
    ShapeCache cache = get_shape_cache(shape);

    Matrix4 model = matrix4_trs(transform->position,
                                transform->rotation,
                                transform->scale);

    // Matrix4 mvp = model * Camera_Data.vp;
    Matrix4 mvp = Camera_Data.vp * model;

    use_shader(mat->shader);

    glBindVertexArray(cache.vao);

    material_set_matrix(mat, "model", model);
    material_set_matrix(mat, "mvp", mvp);

    glDrawElements(GL_TRIANGLES, shape->index_count, GL_UNSIGNED_SHORT, 0);

    return RENDER_OK;
}

void material_set_matrix(Material* mat, String name, Matrix4 data) {
    s32 location = material_get_uniform_location(mat, name);

    glUniformMatrix4fv(location, 1, GL_FALSE, data.e);
}

void material_set_matrix(Material* mat, u32 location, Matrix4 data) {
    glUniformMatrix4fv(location, 1, GL_FALSE, data.e);
}

s32 material_get_uniform_location(Material* mat, String name) {
    Assertf(table_contains(&mat->uniforms, name), "Shader does not contains uniform with name %s", name.text);

    return table_get(&mat->uniforms, name);
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
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, Camera_UBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, Time_UBO);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * shape->vertex_count, shape->vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebobo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * shape->index_count, shape->indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
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