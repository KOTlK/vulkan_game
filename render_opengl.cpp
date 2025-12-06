#include "glass.h"
#include "render.h"
#include "debug.h"
#include "glad/glad.h"

#ifdef GLASS_SDL
#include "glass_sdl.h"
#endif

struct RenderContext {
#ifdef GLASS_SDL
    SDL_GLContext sdl_context;
#endif
};

static RenderContext Context{};

RenderError render_init(Window* window) {
    int glad_version = 0;
#ifdef GLASS_SDL
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    Context.sdl_context  = SDL_GL_CreateContext(window->window);
    glad_version = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
#endif

    if (glad_version == 0) {
        Errf("Cannot load glad.");
        return RENDER_INTERNAL_ERROR;
    }

    Logf("Glad version: %i.", glad_version);

    return RENDER_OK;
}

void render_destroy() {
#ifdef GLASS_SDL
    SDL_GL_DestroyContext(Context.sdl_context);
#endif
}

RenderError render_test() {
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    return RENDER_OK;
}