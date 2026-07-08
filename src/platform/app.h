#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*GameAppPollEventsFn)(void *user_data);
typedef void (*GameAppRenderHookFn)(void *user_data);
typedef void (*GameAppPlatformRenderFn)(void *user_data, void *renderer);
typedef void (*GameAppPlatformEventFn)(void *user_data, void *event, int viewport_width, int viewport_height);

typedef struct GameAppConfig {
    const char *title;
    int window_width;
    int window_height;
    double fixed_dt;
    int max_ticks;

    void *user_data;
    void *platform_user_data;
    void (*on_init)(void *user_data);
    void (*on_update)(void *user_data, double dt);
    void (*on_render)(void *user_data, double interpolation);
    void (*on_shutdown)(void *user_data);
    GameAppPollEventsFn on_poll_events;
    GameAppRenderHookFn on_pre_render;
    GameAppRenderHookFn on_post_render;
    GameAppPlatformRenderFn on_platform_render;
    GameAppPlatformEventFn on_platform_event;
} GameAppConfig;

int game_app_run(const GameAppConfig *cfg);

#ifdef GAME_WITH_SDL3
int game_app_run_sdl(const GameAppConfig *cfg);
#endif

#ifdef __cplusplus
}
#endif
