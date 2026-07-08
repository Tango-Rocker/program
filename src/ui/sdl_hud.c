#ifdef GAME_WITH_SDL3

#include "ui/sdl_hud.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "ui/command_inspector.h"

static SDL_Color color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (SDL_Color){r, g, b, a};
}

static SDL_FColor color_to_fcolor(SDL_Color color)
{
    const float scale = 1.0f / 255.0f;
    return (SDL_FColor){
        .r = (float)color.r * scale,
        .g = (float)color.g * scale,
        .b = (float)color.b * scale,
        .a = (float)color.a * scale,
    };
}

static bool read_tile_field_value(const GameTileField *field, GameHexAxial tile, int32_t *out_value)
{
    if (!field || !field->values || !out_value || !game_tile_field_contains(field, tile)) {
        return false;
    }

    size_t q = (size_t)(tile.q - field->config.q_min);
    size_t r = (size_t)(tile.r - field->config.r_min);
    *out_value = field->values[q * field->config.r_count + r];
    return true;
}

static void draw_rect(SDL_Renderer *renderer, GameUiRect rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect sdl_rect = {.x = rect.x, .y = rect.y, .w = rect.w, .h = rect.h};
    SDL_RenderFillRect(renderer, &sdl_rect);
}

static void draw_rect_outline(SDL_Renderer *renderer, GameUiRect rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect sdl_rect = {.x = rect.x, .y = rect.y, .w = rect.w, .h = rect.h};
    SDL_RenderRect(renderer, &sdl_rect);
}

static bool ensure_font(GameSdlHud *hud)
{
    if (!hud) {
        return false;
    }
    if (hud->initialized) {
        return hud->font != NULL;
    }

    hud->initialized = true;
    if (!TTF_Init()) {
        if (!hud->font_warning_logged) {
            GAME_LOG_WARN("SDL_ttf init failed: %s", SDL_GetError());
            hud->font_warning_logged = true;
        }
        return false;
    }

    hud->font = TTF_OpenFont("C:/Windows/Fonts/segoeui.ttf", 20.0f);
    if (!hud->font) {
        hud->font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 20.0f);
    }
    if (!hud->font && !hud->font_warning_logged) {
        GAME_LOG_WARN("No UI font available; rendering shape-only HUD.");
        hud->font_warning_logged = true;
    }
    return hud->font != NULL;
}

static bool same_color(SDL_Color a, SDL_Color b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static void destroy_text_cache_entry(GameSdlHudTextCacheEntry *entry)
{
    if (!entry) {
        return;
    }
    if (entry->texture) {
        SDL_DestroyTexture(entry->texture);
    }
    *entry = (GameSdlHudTextCacheEntry){0};
}

static void clear_text_cache(GameSdlHud *hud)
{
    if (!hud) {
        return;
    }
    for (size_t i = 0u; i < GAME_SDL_HUD_TEXT_CACHE_CAPACITY; ++i) {
        destroy_text_cache_entry(&hud->text_cache[i]);
    }
    hud->next_text_cache_slot = 0u;
    hud->text_renderer = NULL;
}

static GameSdlHudTextCacheEntry *find_text_cache_entry(GameSdlHud *hud, const char *text, SDL_Color color)
{
    if (!hud || !text) {
        return NULL;
    }
    for (size_t i = 0u; i < GAME_SDL_HUD_TEXT_CACHE_CAPACITY; ++i) {
        GameSdlHudTextCacheEntry *entry = &hud->text_cache[i];
        if (entry->in_use && same_color(entry->color, color) && strcmp(entry->text, text) == 0) {
            return entry;
        }
    }
    return NULL;
}

static GameSdlHudTextCacheEntry *cache_text_entry(SDL_Renderer *renderer, GameSdlHud *hud, const char *text,
                                                  SDL_Color color)
{
    if (!renderer || !hud || !text) {
        return NULL;
    }

    size_t slot = hud->next_text_cache_slot % GAME_SDL_HUD_TEXT_CACHE_CAPACITY;
    hud->next_text_cache_slot = (slot + 1u) % GAME_SDL_HUD_TEXT_CACHE_CAPACITY;
    GameSdlHudTextCacheEntry *entry = &hud->text_cache[slot];
    destroy_text_cache_entry(entry);

    SDL_Surface *surface = TTF_RenderText_Blended(hud->font, text, strlen(text), color);
    if (!surface) {
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return NULL;
    }

    entry->in_use = true;
    (void)snprintf(entry->text, sizeof(entry->text), "%s", text);
    entry->color = color;
    entry->texture = texture;
    entry->width = surface->w;
    entry->height = surface->h;
    SDL_DestroySurface(surface);
    return entry;
}

static void draw_text(SDL_Renderer *renderer, GameSdlHud *hud, const char *text, float x, float y, SDL_Color color)
{
    if (!renderer || !text || text[0] == '\0' || !ensure_font(hud)) {
        return;
    }

    if (hud->text_renderer != renderer) {
        clear_text_cache(hud);
        hud->text_renderer = renderer;
    }
    GameSdlHudTextCacheEntry *entry = find_text_cache_entry(hud, text, color);
    if (!entry) {
        entry = cache_text_entry(renderer, hud, text, color);
    }
    if (!entry || !entry->texture) {
        return;
    }

    SDL_FRect dst = {
        .x = x,
        .y = y,
        .w = (float)entry->width,
        .h = (float)entry->height,
    };
    SDL_RenderTexture(renderer, entry->texture, NULL, &dst);
}

static void draw_textf(SDL_Renderer *renderer, GameSdlHud *hud, float x, float y, SDL_Color color, const char *format,
                       ...)
{
    char buffer[256] = {0};
    va_list args;
    va_start(args, format);
    (void)vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    draw_text(renderer, hud, buffer, x, y, color);
}

static void draw_hex(SDL_Renderer *renderer, float cx, float cy, float radius, SDL_Color color)
{
    SDL_Vertex vertices[7] = {0};
    SDL_FColor fcolor = color_to_fcolor(color);
    const float px[6] = {0.0f, 0.866f, 0.866f, 0.0f, -0.866f, -0.866f};
    const float py[6] = {-1.0f, -0.5f, 0.5f, 1.0f, 0.5f, -0.5f};
    vertices[0].position = (SDL_FPoint){cx, cy};
    vertices[0].color = fcolor;
    for (int i = 0; i < 6; ++i) {
        vertices[i + 1].position = (SDL_FPoint){cx + px[i] * radius, cy + py[i] * radius};
        vertices[i + 1].color = fcolor;
    }
    const int indices[18] = {
        0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 1,
    };
    SDL_RenderGeometry(renderer, NULL, vertices, 7, indices, 18);
}

static void draw_marker(SDL_Renderer *renderer, const GameUiLayout *layout, GameHexAxial tile, SDL_Color color,
                        float size)
{
    float cx = 0.0f;
    float cy = 0.0f;
    game_ui_tile_center(layout, tile, &cx, &cy);
    draw_rect(renderer, (GameUiRect){cx - size * 0.5f, cy - size * 0.5f, size, size}, color);
}

static void draw_centered_label(SDL_Renderer *renderer, GameSdlHud *hud, GameUiRect rect, const char *label,
                                SDL_Color color)
{
    size_t length = label ? strlen(label) : 0u;
    float estimated_w = (float)length * 10.0f;
    float x = rect.x + (rect.w - estimated_w) * 0.5f;
    if (x < rect.x + 4.0f) {
        x = rect.x + 4.0f;
    }
    draw_text(renderer, hud, label, x, rect.y + (rect.h - 24.0f) * 0.5f, color);
}

static void draw_path_preview(SDL_Renderer *renderer, const GameUiLayout *layout, const GameDefaultScene *scene)
{
    if (!scene->has_path_preview || scene->path_preview_length == 0u) {
        return;
    }

    for (size_t i = 0u; i < scene->path_preview_length; ++i) {
        float cx = 0.0f;
        float cy = 0.0f;
        game_ui_tile_center(layout, scene->path_preview[i], &cx, &cy);
        if (cx < -layout->hex_step_x || cy < layout->top_bar.h - layout->hex_step_y ||
            cx > layout->right_panel.x + layout->hex_step_x || cy > layout->bottom_panel.y + layout->hex_step_y) {
            continue;
        }
        SDL_Color color = scene->party_following_path ? color_rgba(96, 205, 235, 210) : color_rgba(220, 198, 88, 210);
        draw_rect(renderer, (GameUiRect){cx - 3.0f, cy - 3.0f, 6.0f, 6.0f}, color);
    }
}

static void draw_world(SDL_Renderer *renderer, const GameUiLayout *layout, const GameUiState *ui,
                       const GameDefaultScene *scene)
{
    int32_t min_q = 0;
    int32_t max_q = 0;
    int32_t min_r = 0;
    int32_t max_r = 0;
    game_ui_visible_tile_bounds(layout, scene, &min_q, &max_q, &min_r, &max_r);
    for (int32_t r = min_r; r <= max_r; ++r) {
        for (int32_t q = min_q; q <= max_q; ++q) {
            GameHexAxial tile = {q, r};
            float cx = 0.0f;
            float cy = 0.0f;
            game_ui_tile_center(layout, tile, &cx, &cy);
            if (cx < -layout->hex_step_x || cy < layout->top_bar.h - layout->hex_step_y ||
                cx > layout->right_panel.x + layout->hex_step_x || cy > layout->bottom_panel.y + layout->hex_step_y) {
                continue;
            }

            int32_t value = -1;
            (void)game_world_map_get(&scene->scenario.map, tile, &value);

            int32_t noise = 0;
            (void)read_tile_field_value(&scene->noise_field, tile, &noise);
            SDL_Color color = value < 0 ? color_rgba(44, 49, 58, 255) : color_rgba(54, 118, 82, 255);
            if (ui->field_overlay_visible && noise > 0) {
                uint8_t pulse = noise > 50 ? 190u : (uint8_t)(90 + noise * 2);
                color = color_rgba(70, pulse, 165, 255);
            }
            if (game_ui_hit_kind_is_world(ui->hover.kind) && ui->hover.tile.q == q && ui->hover.tile.r == r) {
                color = ui->active_action == GAME_UI_ACTION_NONE ? color_rgba(86, 132, 156, 255)
                                                                 : color_rgba(210, 178, 88, 255);
            }
            if (ui->has_selection && ui->selected_tile.q == q && ui->selected_tile.r == r) {
                color = color_rgba(210, 205, 120, 255);
            }

            draw_hex(renderer, cx, cy, layout->hex_radius, color);
            draw_hex(renderer, cx, cy, layout->hex_radius - 2.0f, color);
        }
    }

    draw_path_preview(renderer, layout, scene);
    draw_marker(renderer, layout, scene->party_position, color_rgba(245, 222, 90, 255), 18.0f);
    draw_marker(renderer, layout, scene->scenario.colony_anchor, color_rgba(80, 180, 240, 255), 18.0f);
    draw_marker(renderer, layout, scene->scenario.horde_anchor, color_rgba(174, 96, 232, 255), 20.0f);
    draw_marker(renderer, layout, scene->worker.position, color_rgba(232, 236, 226, 255), 11.0f);
    const GameDefaultSceneDemoStatus *demo = game_default_scene_demo_status(scene);
    if (demo && demo->horde_materialized) {
        draw_marker(renderer, layout, demo->horde_spawn_tile, color_rgba(235, 80, 68, 255), 12.0f);
    }
}

static void minimap_point(const GameUiLayout *layout, const GameDefaultScene *scene, GameHexAxial tile, float *out_x,
                          float *out_y)
{
    int32_t max_q = scene->scenario.horde_anchor.q > 0 ? scene->scenario.horde_anchor.q : 1;
    int32_t max_r = scene->scenario.horde_anchor.r > 0 ? scene->scenario.horde_anchor.r : 1;
    if (out_x) {
        *out_x = layout->minimap.x + ((float)tile.q / (float)max_q) * layout->minimap.w;
    }
    if (out_y) {
        *out_y = layout->minimap.y + ((float)tile.r / (float)max_r) * layout->minimap.h;
    }
}

static void draw_minimap_marker(SDL_Renderer *renderer, const GameUiLayout *layout, const GameDefaultScene *scene,
                                GameHexAxial tile, SDL_Color color, float size)
{
    float x = 0.0f;
    float y = 0.0f;
    minimap_point(layout, scene, tile, &x, &y);
    draw_rect(renderer, (GameUiRect){x - size * 0.5f, y - size * 0.5f, size, size}, color);
}

static void draw_minimap(SDL_Renderer *renderer, GameSdlHud *hud, const GameUiLayout *layout, const GameUiState *ui,
                         const GameDefaultScene *scene)
{
    draw_rect(renderer, layout->minimap, color_rgba(10, 13, 18, 255));
    draw_rect_outline(renderer, layout->minimap, color_rgba(74, 82, 92, 255));

    int32_t q_step = 1;
    int32_t r_step = 1;
    game_ui_minimap_sample_stride(layout, scene, &q_step, &r_step);
    for (int32_t q = 0; q <= scene->scenario.horde_anchor.q; q += q_step) {
        for (int32_t r = 0; r <= scene->scenario.horde_anchor.r; r += r_step) {
            GameHexAxial tile = {q, r};
            int32_t terrain = -1;
            (void)game_world_map_get(&scene->scenario.map, tile, &terrain);
            float x = 0.0f;
            float y = 0.0f;
            minimap_point(layout, scene, tile, &x, &y);
            SDL_Color color = terrain < 0 ? color_rgba(42, 46, 54, 210) : color_rgba(45, 96, 70, 210);
            int32_t noise = 0;
            if (ui->minimap_noise_visible && read_tile_field_value(&scene->noise_field, tile, &noise) && noise > 0) {
                color = color_rgba(50, 185, 130, 230);
            }
            draw_rect(renderer, (GameUiRect){x, y, 1.0f, 1.0f}, color);
        }
    }

    draw_minimap_marker(renderer, layout, scene, scene->party_position, color_rgba(245, 222, 90, 255), 5.0f);
    draw_minimap_marker(renderer, layout, scene, scene->scenario.colony_anchor, color_rgba(80, 180, 240, 255), 4.0f);
    draw_minimap_marker(renderer, layout, scene, scene->scenario.horde_anchor, color_rgba(174, 96, 232, 255), 5.0f);

    float camera_x = 0.0f;
    float camera_y = 0.0f;
    minimap_point(layout, scene, ui->camera_center, &camera_x, &camera_y);
    float view_w = layout->minimap.w * 0.18f;
    float view_h = layout->minimap.h * 0.22f;
    draw_rect_outline(renderer, (GameUiRect){camera_x - view_w * 0.5f, camera_y - view_h * 0.5f, view_w, view_h},
                      color_rgba(230, 234, 228, 255));

    draw_rect(renderer, layout->focus_party_button,
              ui->selection_kind == GAME_UI_SELECTION_PARTY ? color_rgba(88, 82, 42, 255)
                                                            : color_rgba(42, 48, 60, 255));
    draw_text(renderer, hud, "P", layout->focus_party_button.x + 9.0f, layout->focus_party_button.y + 5.0f,
              color_rgba(245, 222, 90, 255));
    draw_rect(renderer, layout->focus_colony_button,
              ui->selection_kind == GAME_UI_SELECTION_COLONY ? color_rgba(42, 78, 96, 255)
                                                             : color_rgba(42, 48, 60, 255));
    draw_text(renderer, hud, "C", layout->focus_colony_button.x + 9.0f, layout->focus_colony_button.y + 5.0f,
              color_rgba(80, 180, 240, 255));
    draw_rect(renderer, layout->focus_horde_button,
              ui->selection_kind == GAME_UI_SELECTION_HORDE ? color_rgba(74, 48, 92, 255)
                                                            : color_rgba(42, 48, 60, 255));
    draw_text(renderer, hud, "H", layout->focus_horde_button.x + 9.0f, layout->focus_horde_button.y + 5.0f,
              color_rgba(174, 96, 232, 255));
    draw_rect(renderer, layout->minimap_filter_button,
              ui->minimap_noise_visible ? color_rgba(50, 112, 88, 255) : color_rgba(42, 48, 60, 255));
    draw_text(renderer, hud, "N", layout->minimap_filter_button.x + 9.0f, layout->minimap_filter_button.y + 5.0f,
              color_rgba(130, 220, 170, 255));
}

static const char *posture_label(GameHordePosture posture)
{
    switch (posture) {
    case GAME_HORDE_POSTURE_THREATENED:
        return "Threatened";
    case GAME_HORDE_POSTURE_ALERT:
        return "Alert";
    default:
        return "Dormant";
    }
}

static void draw_event_strip(SDL_Renderer *renderer, GameSdlHud *hud, const GameUiLayout *layout,
                             const GameDefaultScene *scene)
{
    draw_rect(renderer, layout->event_strip, color_rgba(20, 23, 30, 230));
    draw_text(renderer, hud, "Latest", layout->event_strip.x + 12.0f, layout->event_strip.y + 10.0f,
              color_rgba(220, 224, 220, 255));
    const char *message = scene->alert_count > 0u ? scene->alerts[scene->alert_count - 1u] : "No urgent alerts";
    draw_text(renderer, hud, message, layout->event_strip.x + 86.0f, layout->event_strip.y + 10.0f,
              color_rgba(210, 188, 118, 255));
}

static void draw_top_bar(SDL_Renderer *renderer, GameSdlHud *hud, const GameUiLayout *layout,
                         const GameDefaultScene *scene, const GameUiState *ui)
{
    draw_rect(renderer, layout->top_bar, color_rgba(13, 15, 21, 255));
    draw_textf(renderer, hud, 22.0f, 24.0f, color_rgba(235, 238, 232, 255), "Party q%d r%d    Horde %s    Pressure %u",
               scene->party_position.q, scene->party_position.r, posture_label(scene->horde_attention.posture),
               scene->horde_attention.pressure);
    draw_rect(renderer, layout->pause_button, ui->paused ? color_rgba(138, 74, 62, 255) : color_rgba(42, 48, 60, 255));
    draw_text(renderer, hud, ui->paused ? "Run" : "Pause", layout->pause_button.x + 10.0f,
              layout->pause_button.y + 8.0f, color_rgba(230, 234, 228, 255));
    draw_rect(renderer, layout->speed_button, color_rgba(42, 48, 60, 255));
    draw_textf(renderer, hud, layout->speed_button.x + 15.0f, layout->speed_button.y + 8.0f,
               color_rgba(230, 234, 228, 255), "%ux", ui->speed_multiplier == 0u ? 1u : ui->speed_multiplier);
}

static void draw_bottom_panel(SDL_Renderer *renderer, GameSdlHud *hud, const GameUiLayout *layout,
                              const GameUiState *ui, const GameDefaultScene *scene)
{
    draw_rect(renderer, layout->bottom_panel, color_rgba(17, 20, 27, 245));
    draw_minimap(renderer, hud, layout, ui, scene);
    float info_x = layout->focus_horde_button.x + layout->focus_horde_button.w + 24.0f;
    draw_text(renderer, hud, "Party", info_x, layout->bottom_panel.y + 24.0f, color_rgba(230, 232, 224, 255));
    draw_rect(renderer, (GameUiRect){info_x + 78.0f, layout->bottom_panel.y + 22.0f, 42.0f, 42.0f},
              color_rgba(245, 222, 90, 255));
    draw_textf(renderer, hud, info_x + 138.0f, layout->bottom_panel.y + 22.0f, color_rgba(205, 214, 210, 255),
               "q%d r%d", scene->party_position.q, scene->party_position.r);
    draw_textf(renderer, hud, info_x + 138.0f, layout->bottom_panel.y + 50.0f, color_rgba(168, 180, 178, 255), "%s",
               scene->path_summary[0] != '\0' ? scene->path_summary : "Choose an action, then pick a world target");

    draw_rect(renderer, layout->move_button,
              ui->active_action == GAME_UI_ACTION_MOVE ? color_rgba(70, 124, 180, 255) : color_rgba(44, 88, 138, 255));
    draw_rect_outline(renderer, layout->move_button, color_rgba(132, 178, 230, 255));
    draw_centered_label(renderer, hud, layout->move_button, "Move", color_rgba(245, 248, 240, 255));

    draw_rect(renderer, layout->interact_button,
              ui->active_action == GAME_UI_ACTION_INTERACT ? color_rgba(150, 128, 58, 255)
                                                           : color_rgba(108, 94, 46, 255));
    draw_rect_outline(renderer, layout->interact_button, color_rgba(216, 190, 92, 255));
    draw_centered_label(renderer, hud, layout->interact_button, "Interact", color_rgba(245, 248, 240, 255));

    draw_rect(renderer, layout->attack_button,
              ui->active_action == GAME_UI_ACTION_ATTACK ? color_rgba(162, 74, 64, 255) : color_rgba(112, 56, 52, 255));
    draw_rect_outline(renderer, layout->attack_button, color_rgba(224, 120, 104, 255));
    draw_centered_label(renderer, hud, layout->attack_button, "Attack", color_rgba(245, 248, 240, 255));

    draw_rect(renderer, layout->emit_noise_button,
              ui->active_action == GAME_UI_ACTION_NOISE ? color_rgba(55, 168, 116, 255) : color_rgba(42, 132, 96, 255));
    draw_rect_outline(renderer, layout->emit_noise_button, color_rgba(130, 220, 170, 255));
    draw_centered_label(renderer, hud, layout->emit_noise_button, "Noise", color_rgba(245, 248, 240, 255));

    const char *result =
        ui->has_command_result ? game_command_inspector_result_message(ui->last_command_result) : "Ready";
    draw_textf(renderer, hud, layout->emit_noise_button.x + layout->emit_noise_button.w + 24.0f,
               layout->emit_noise_button.y + 14.0f, color_rgba(205, 214, 210, 255), "%s", result);
    draw_textf(renderer, hud, layout->emit_noise_button.x + layout->emit_noise_button.w + 24.0f,
               layout->emit_noise_button.y + 40.0f, color_rgba(158, 174, 174, 255), "Action: %s",
               game_ui_action_label(ui->active_action));
}

static void draw_right_panel(SDL_Renderer *renderer, GameSdlHud *hud, const GameUiLayout *layout, const GameUiState *ui,
                             const GameDefaultScene *scene)
{
    draw_rect(renderer, layout->right_panel, color_rgba(18, 21, 28, 238));
    float x = layout->right_panel.x + 22.0f;
    float y = layout->right_panel.y + 22.0f;
    draw_text(renderer, hud, "Target", x, y, color_rgba(235, 238, 232, 255));
    y += 38.0f;

    GameHexAxial selected = ui->has_selection ? ui->selected_tile : scene->party_position;
    int32_t terrain = -1;
    int32_t noise = 0;
    (void)game_world_map_get(&scene->scenario.map, selected, &terrain);
    (void)read_tile_field_value(&scene->noise_field, selected, &noise);
    draw_textf(renderer, hud, x, y, color_rgba(205, 212, 208, 255), "%s at q%d r%d",
               game_ui_selection_label(ui->selection_kind), selected.q, selected.r);
    y += 34.0f;
    draw_textf(renderer, hud, x, y, color_rgba(178, 190, 188, 255), "Ground: %s", terrain < 0 ? "blocked" : "open");
    y += 30.0f;
    draw_textf(renderer, hud, x, y, color_rgba(178, 190, 188, 255), "Noise: %d", noise);
    y += 30.0f;
    draw_textf(renderer, hud, x, y, color_rgba(178, 190, 188, 255), "Order: %s",
               scene->party_following_path ? "moving" : game_ui_action_label(ui->active_action));
    y += 38.0f;
    if (scene->interaction_summary[0] != '\0') {
        draw_text(renderer, hud, scene->interaction_summary, x, y, color_rgba(210, 188, 118, 255));
        y += 30.0f;
    }
    if (scene->attack_summary[0] != '\0') {
        draw_text(renderer, hud, scene->attack_summary, x, y, color_rgba(220, 150, 142, 255));
        y += 30.0f;
    }
    if (scene->alert_count > 0u) {
        draw_text(renderer, hud, scene->alerts[scene->alert_count - 1u], x, y, color_rgba(224, 196, 116, 255));
    }

    draw_rect(renderer, layout->forensic_toggle,
              ui->forensic_expanded ? color_rgba(80, 92, 115, 255) : color_rgba(42, 48, 60, 255));
    draw_centered_label(renderer, hud, layout->forensic_toggle, ui->forensic_expanded ? "Hide Why" : "Why",
                        color_rgba(230, 234, 228, 255));
    draw_rect(renderer, layout->field_toggle,
              ui->field_overlay_visible ? color_rgba(50, 112, 88, 255) : color_rgba(42, 48, 60, 255));
    draw_centered_label(renderer, hud, layout->field_toggle, "Noise Map", color_rgba(230, 234, 228, 255));

    if (ui->forensic_expanded) {
        draw_rect(renderer, layout->causal_panel, color_rgba(12, 14, 20, 238));
        float line_y = layout->causal_panel.y + 14.0f;
        const char *cursor = scene->causal_report;
        for (int line = 0; line < 4 && cursor && *cursor != '\0'; ++line) {
            char text[96] = {0};
            size_t i = 0u;
            while (cursor[i] != '\0' && cursor[i] != '\n' && i + 1u < sizeof(text)) {
                text[i] = cursor[i];
                ++i;
            }
            draw_text(renderer, hud, text, layout->causal_panel.x + 12.0f, line_y, color_rgba(170, 184, 184, 255));
            line_y += 26.0f;
            cursor = cursor[i] == '\n' ? cursor + i + 1u : cursor + i;
        }
    }
}

static void draw_tooltip(SDL_Renderer *renderer, GameSdlHud *hud, const GameUiLayout *layout, const GameUiState *ui)
{
    if (!ui || ui->hover.kind == GAME_UI_HIT_NONE || ui->hover.kind == GAME_UI_HIT_CHROME) {
        return;
    }

    float x = layout->right_panel.x - 188.0f;
    float y = layout->bottom_panel.y - 34.0f;
    if (x < 12.0f) {
        x = 12.0f;
    }
    draw_rect(renderer, (GameUiRect){x, y, 176.0f, 26.0f}, color_rgba(12, 14, 20, 230));
    draw_rect_outline(renderer, (GameUiRect){x, y, 176.0f, 26.0f}, color_rgba(92, 102, 116, 255));
    draw_textf(renderer, hud, x + 8.0f, y + 5.0f, color_rgba(220, 226, 220, 255), "%s q%d r%d",
               game_ui_hit_label(ui->hover.kind), ui->hover.tile.q, ui->hover.tile.r);
}

static void draw_menu_button(SDL_Renderer *renderer, GameSdlHud *hud, GameUiRect rect, const char *label,
                             bool highlighted)
{
    SDL_Color fill = highlighted ? color_rgba(68, 92, 118, 255) : color_rgba(38, 45, 58, 255);
    draw_rect(renderer, rect, fill);
    draw_rect_outline(renderer, rect, color_rgba(106, 124, 146, 255));
    draw_centered_label(renderer, hud, rect, label, color_rgba(236, 240, 232, 255));
}

static void draw_menu_line(SDL_Renderer *renderer, GameSdlHud *hud, const GameUiLayout *layout, int line,
                           const char *text)
{
    float x = layout->main_menu_panel.x + 34.0f;
    float y = layout->main_menu_panel.y + 98.0f + (float)line * 24.0f;
    draw_text(renderer, hud, text, x, y, color_rgba(190, 202, 202, 255));
}

static void draw_main_menu(SDL_Renderer *renderer, GameSdlHud *hud, const GameUiLayout *layout, const GameUiState *ui)
{
    draw_rect(renderer, (GameUiRect){0.0f, 0.0f, (float)layout->viewport_width, (float)layout->viewport_height},
              color_rgba(12, 14, 18, 255));
    draw_rect(renderer, layout->main_menu_panel, color_rgba(18, 23, 30, 255));
    draw_rect_outline(renderer, layout->main_menu_panel, color_rgba(88, 102, 116, 255));
    draw_text(renderer, hud, "Autonomous Simulation", layout->main_menu_panel.x + 34.0f,
              layout->main_menu_panel.y + 28.0f, color_rgba(238, 242, 234, 255));
    draw_text(renderer, hud, game_ui_screen_label(ui->screen), layout->main_menu_panel.x + 34.0f,
              layout->main_menu_panel.y + 58.0f, color_rgba(142, 172, 182, 255));

    if (ui->screen == GAME_UI_SCREEN_MENU) {
        draw_menu_button(renderer, hud, layout->menu_new_game_button, "Enter World",
                         ui->hover.kind == GAME_UI_HIT_MENU_NEW_GAME);
        draw_menu_button(renderer, hud, layout->menu_regenerate_button, "Regenerate",
                         ui->hover.kind == GAME_UI_HIT_MENU_REGENERATE);
        draw_menu_button(renderer, hud, layout->menu_load_button, "Load", ui->hover.kind == GAME_UI_HIT_MENU_LOAD);
        draw_menu_button(renderer, hud, layout->menu_settings_button, "Settings",
                         ui->hover.kind == GAME_UI_HIT_MENU_SETTINGS);
        draw_menu_button(renderer, hud, layout->menu_tutorial_button, "Tutorial",
                         ui->hover.kind == GAME_UI_HIT_MENU_TUTORIAL);
        return;
    }

    if (ui->screen == GAME_UI_SCREEN_LOAD) {
        draw_menu_line(renderer, hud, layout, 0, "No saved games are available yet.");
        draw_menu_line(renderer, hud, layout, 1, "Save/load exists as core snapshot work,");
        draw_menu_line(renderer, hud, layout, 2, "but this SDL menu has no save slots.");
    } else if (ui->screen == GAME_UI_SCREEN_SETTINGS) {
        draw_menu_line(renderer, hud, layout, 0, "Current settings are runtime controls.");
        draw_menu_line(renderer, hud, layout, 1, "Pause and speed are in the top bar.");
        draw_menu_line(renderer, hud, layout, 2, "Press M in game to return here.");
    } else if (ui->screen == GAME_UI_SCREEN_TUTORIAL) {
        draw_menu_line(renderer, hud, layout, 0, "Select tiles and markers with left click.");
        draw_menu_line(renderer, hud, layout, 1, "Use Move or right click to send the party.");
        draw_menu_line(renderer, hud, layout, 2, "Noise, attacks, and interactions feed logs,");
        draw_menu_line(renderer, hud, layout, 3, "fields, horde attention, alerts, and reports.");
        draw_menu_line(renderer, hud, layout, 4, "Use the inspector and causal panel to read why.");
    }
    draw_menu_button(renderer, hud, layout->menu_back_button, "Back", ui->hover.kind == GAME_UI_HIT_MENU_BACK);
}

void game_sdl_hud_init(GameSdlHud *hud)
{
    if (!hud) {
        return;
    }
    *hud = (GameSdlHud){0};
}

void game_sdl_hud_destroy(GameSdlHud *hud)
{
    if (!hud) {
        return;
    }
    clear_text_cache(hud);
    if (hud->font) {
        TTF_CloseFont(hud->font);
        hud->font = NULL;
    }
    if (hud->initialized) {
        TTF_Quit();
    }
    *hud = (GameSdlHud){0};
}

void game_sdl_hud_render(GameSdlHud *hud, SDL_Renderer *renderer, const GameUiState *ui, const GameDefaultScene *scene,
                         uint64_t app_tick)
{
    if (!hud || !renderer || !ui || !scene) {
        return;
    }

    int width = 0;
    int height = 0;
    SDL_Window *window = SDL_GetRenderWindow(renderer);
    if (window) {
        (void)SDL_GetWindowSize(window, &width, &height);
    }
    if (width <= 0 || height <= 0) {
        (void)SDL_GetCurrentRenderOutputSize(renderer, &width, &height);
    }
    GameUiLayout layout = {0};
    game_ui_layout_build(width, height, &layout);
    game_ui_layout_set_camera(&layout, ui->camera_center);

    (void)app_tick;
    if (ui->screen != GAME_UI_SCREEN_GAME) {
        draw_main_menu(renderer, hud, &layout, ui);
        return;
    }

    draw_top_bar(renderer, hud, &layout, scene, ui);
    draw_world(renderer, &layout, ui, scene);
    draw_event_strip(renderer, hud, &layout, scene);
    draw_bottom_panel(renderer, hud, &layout, ui, scene);
    draw_right_panel(renderer, hud, &layout, ui, scene);
    draw_tooltip(renderer, hud, &layout, ui);
}

#endif
