#pragma once

#include <stddef.h>

#include <SDL3/SDL.h>

#include "event/event_log.h"

#ifdef __cplusplus
extern "C" {
#endif

void game_debug_overlay_render_events(
    SDL_Renderer *renderer,
    const GameEventLog *event_log,
    size_t max_entries
);

#ifdef __cplusplus
}
#endif
