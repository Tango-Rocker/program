#include "ui/debug_overlay.h"

#include <stdio.h>

static SDL_Color game_debug_overlay_type_color(GameEventType type) {
    if (type == GAME_EVENT_TYPE_NOISE_EMITTED) {
        return (SDL_Color){40, 200, 110, 255};
    }
    if (type == GAME_EVENT_TYPE_FIELD_IMPULSE_APPLIED) {
        return (SDL_Color){220, 190, 50, 255};
    }
    if (type == GAME_EVENT_TYPE_ACTOR_ATTENTION_UPDATED) {
        return (SDL_Color){150, 80, 230, 255};
    }
    return (SDL_Color){180, 180, 180, 255};
}

void game_debug_overlay_render_events(SDL_Renderer *renderer, const GameEventLog *event_log, size_t max_entries) {
    if (!renderer || !event_log || max_entries == 0u) {
        return;
    }

    const size_t count = game_event_log_count(event_log);
    if (count == 0u) {
        return;
    }

    size_t visible_count = count < max_entries ? count : max_entries;
    const size_t first = count - visible_count;
    const float bar_height = 8.0f;
    const float bar_gap = 4.0f;
    const float base_x = 10.0f;
    const float base_y = 10.0f;

    size_t index = 0u;
    for (size_t i = first; i < count; ++i) {
        const GameEventLogEntry *entry = game_event_log_at(event_log, i);
        if (!entry) {
            continue;
        }

        SDL_Color color = game_debug_overlay_type_color(entry->type);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        SDL_FRect bar = {0};
        bar.x = base_x;
        bar.y = base_y + (float)index * (bar_height + bar_gap);
        bar.w = 120.0f;
        bar.h = bar_height;
        SDL_RenderFillRect(renderer, &bar);
        ++index;
    }
}
