#ifdef GAME_WITH_SDL3

#include "core/log.h"
#include "platform/app.h"

#include <SDL3/SDL.h>

typedef struct {
    SDL_Renderer *renderer;
    SDL_Window *window;
    void *scene_user_data;
    GameAppPlatformRenderFn scene_render;
    GameAppPlatformEventFn scene_event;
} SdlRenderContext;

static bool sdl_poll_events(void *user_data)
{
    SdlRenderContext *ctx = (SdlRenderContext *)user_data;

    bool running = true;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
            running = false;
        } else if (ctx && ctx->scene_event) {
            int width = 0;
            int height = 0;
            if (ctx->window) {
                (void)SDL_GetWindowSize(ctx->window, &width, &height);
            }
            if (width <= 0 || height <= 0) {
                (void)SDL_GetCurrentRenderOutputSize(ctx->renderer, &width, &height);
            }
            ctx->scene_event(ctx->scene_user_data, &event, width, height);
        }
    }
    return running;
}

static void sdl_pre_render(void *user_data)
{
    SdlRenderContext *ctx = (SdlRenderContext *)user_data;
    if (!ctx || !ctx->renderer) {
        return;
    }

    SDL_SetRenderDrawColor(ctx->renderer, 18, 18, 26, 255);
    SDL_RenderClear(ctx->renderer);
}

static void sdl_post_render(void *user_data)
{
    SdlRenderContext *ctx = (SdlRenderContext *)user_data;
    if (!ctx || !ctx->renderer) {
        return;
    }
    if (ctx->scene_render) {
        ctx->scene_render(ctx->scene_user_data, ctx->renderer);
    }
    SDL_RenderPresent(ctx->renderer);
}

int game_app_run_sdl(const GameAppConfig *cfg)
{
    if (!cfg) {
        GAME_LOG_ERROR("Game app config is null.");
        return -1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        GAME_LOG_ERROR("SDL init failed: %s", SDL_GetError());
        return -1;
    }

    const int window_width = cfg->window_width > 0 ? cfg->window_width : 1280;
    const int window_height = cfg->window_height > 0 ? cfg->window_height : 720;
    SDL_Window *window = SDL_CreateWindow(cfg->title ? cfg->title : "Autonomous Simulation RTS/RPG", window_width,
                                          window_height, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        GAME_LOG_WARN("Fullscreen window creation failed: %s", SDL_GetError());
        window = SDL_CreateWindow(cfg->title ? cfg->title : "Autonomous Simulation RTS/RPG", window_width,
                                  window_height, SDL_WINDOW_RESIZABLE);
    }
    if (!window) {
        GAME_LOG_ERROR("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        GAME_LOG_ERROR("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SdlRenderContext render_ctx = {
        .renderer = renderer,
        .window = window,
        .scene_user_data = cfg->user_data,
        .scene_render = cfg->on_platform_render,
        .scene_event = cfg->on_platform_event,
    };
    GameAppConfig loop_cfg = *cfg;
    loop_cfg.platform_user_data = &render_ctx;
    loop_cfg.on_poll_events = sdl_poll_events;
    loop_cfg.on_pre_render = sdl_pre_render;
    loop_cfg.on_post_render = sdl_post_render;

    const int result = game_app_run(&loop_cfg);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}

#else

int game_app_run_sdl(void)
{
    (void)0;
    return -1;
}

#endif
