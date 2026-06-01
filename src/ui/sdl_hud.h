#pragma once

#ifdef GAME_WITH_SDL3

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "game/default_scene.h"
#include "ui/ui_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_SDL_HUD_TEXT_CACHE_CAPACITY 64u
#define GAME_SDL_HUD_TEXT_CACHE_CHARS 256u

typedef struct {
    bool in_use;
    char text[GAME_SDL_HUD_TEXT_CACHE_CHARS];
    SDL_Color color;
    SDL_Texture *texture;
    int width;
    int height;
} GameSdlHudTextCacheEntry;

typedef struct {
    bool initialized;
    bool font_warning_logged;
    TTF_Font *font;
    SDL_Renderer *text_renderer;
    size_t next_text_cache_slot;
    GameSdlHudTextCacheEntry text_cache[GAME_SDL_HUD_TEXT_CACHE_CAPACITY];
} GameSdlHud;

void game_sdl_hud_init(GameSdlHud *hud);
void game_sdl_hud_destroy(GameSdlHud *hud);
void game_sdl_hud_render(GameSdlHud *hud, SDL_Renderer *renderer, const GameUiState *ui, const GameDefaultScene *scene,
                         uint64_t app_tick);

#ifdef __cplusplus
}
#endif

#endif
