#include "platform/app.h"

#include "core/log.h"
#include "core/time.h"

#include <stdbool.h>

static const double GAME_FIXED_DT_FALLBACK = 1.0 / 60.0;
static const double GAME_LOOP_MAX_FRAME_SECONDS = 0.25;

int game_app_run(const GameAppConfig *cfg) {
    if (!cfg) {
        GAME_LOG_ERROR("Game app config is null.");
        return -1;
    }

    if (cfg->max_ticks < 0) {
        GAME_LOG_ERROR("GameAppConfig.max_ticks must be >= 0.");
        return -1;
    }

    const double dt = cfg->fixed_dt > 0.0 ? cfg->fixed_dt : GAME_FIXED_DT_FALLBACK;
    const int max_ticks = cfg->max_ticks;
    void (*const init_fn)(void *) = cfg->on_init;
    void (*const update_fn)(void *, double) = cfg->on_update;
    void (*const render_fn)(void *, double) = cfg->on_render;
    void (*const shutdown_fn)(void *) = cfg->on_shutdown;
    const GameAppPollEventsFn poll_fn = cfg->on_poll_events;
    const GameAppRenderHookFn pre_render_fn = cfg->on_pre_render;
    const GameAppRenderHookFn post_render_fn = cfg->on_post_render;
    void *const platform_user_data = cfg->platform_user_data ? cfg->platform_user_data : cfg->user_data;

    bool running = true;
    bool did_init = false;
    int tick_count = 0;
    double last = game_time_now_seconds();
    double accumulator = 0.0;

    GAME_LOG_INFO("Starting fixed timestep loop (dt=%.4f, max_ticks=%d).", dt, max_ticks);

    if (init_fn) {
        init_fn(cfg->user_data);
        did_init = true;
    }

    while (running && (max_ticks == 0 || tick_count < max_ticks)) {
        if (poll_fn) {
            running = poll_fn(platform_user_data);
        }
        if (!running) {
            break;
        }

        const double now = game_time_now_seconds();
        double frame = now - last;
        last = now;
        if (frame < 0.0 || frame > GAME_LOOP_MAX_FRAME_SECONDS) {
            frame = 0.0;
        }

        accumulator += frame;
        while (accumulator >= dt && running) {
            if (update_fn) {
                update_fn(cfg->user_data, dt);
            }
            accumulator -= dt;
            tick_count++;

            if (max_ticks > 0 && tick_count >= max_ticks) {
                running = false;
            }
        }

        if (pre_render_fn) {
            pre_render_fn(platform_user_data);
        }
        if (render_fn) {
            render_fn(cfg->user_data, dt > 0.0 ? accumulator / dt : 0.0);
        }
        if (post_render_fn) {
            post_render_fn(platform_user_data);
        }

        if (!pre_render_fn && !render_fn && !post_render_fn) {
            game_time_sleep_ms(1);
        }
    }

    if (did_init && shutdown_fn) {
        shutdown_fn(cfg->user_data);
    }

    GAME_LOG_INFO("App loop finished (ticks=%d).", tick_count);
    return 0;
}
