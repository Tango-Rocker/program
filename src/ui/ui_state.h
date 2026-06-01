#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game/default_scene.h"
#include "sim/command.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_UI_HIT_NONE = 0,
    GAME_UI_HIT_TILE = 1,
    GAME_UI_HIT_PARTY = 2,
    GAME_UI_HIT_COLONY = 3,
    GAME_UI_HIT_HORDE = 4,
    GAME_UI_HIT_WORKER = 5,
    GAME_UI_HIT_EMIT_NOISE = 6,
    GAME_UI_HIT_FORENSIC_TOGGLE = 7,
    GAME_UI_HIT_FIELD_TOGGLE = 8,
    GAME_UI_HIT_MOVE_PARTY = 9,
    GAME_UI_HIT_INTERACT = 10,
    GAME_UI_HIT_MINIMAP = 11,
    GAME_UI_HIT_FOCUS_PARTY = 12,
    GAME_UI_HIT_FOCUS_COLONY = 13,
    GAME_UI_HIT_FOCUS_HORDE = 14,
    GAME_UI_HIT_ATTACK = 15,
    GAME_UI_HIT_PAUSE_TOGGLE = 16,
    GAME_UI_HIT_SPEED_TOGGLE = 17,
    GAME_UI_HIT_MINIMAP_FILTER = 18,
    GAME_UI_HIT_CHROME = 19,
    GAME_UI_HIT_MENU_NEW_GAME = 20,
    GAME_UI_HIT_MENU_LOAD = 21,
    GAME_UI_HIT_MENU_SETTINGS = 22,
    GAME_UI_HIT_MENU_TUTORIAL = 23,
    GAME_UI_HIT_MENU_BACK = 24,
} GameUiHitKind;

typedef enum {
    GAME_UI_SELECTION_NONE = 0,
    GAME_UI_SELECTION_TILE = 1,
    GAME_UI_SELECTION_PARTY = 2,
    GAME_UI_SELECTION_COLONY = 3,
    GAME_UI_SELECTION_HORDE = 4,
    GAME_UI_SELECTION_WORKER = 5,
} GameUiSelectionKind;

typedef enum {
    GAME_UI_ACTION_NONE = 0,
    GAME_UI_ACTION_MOVE = 1,
    GAME_UI_ACTION_INTERACT = 2,
    GAME_UI_ACTION_NOISE = 3,
    GAME_UI_ACTION_ATTACK = 4,
} GameUiActionMode;

typedef enum {
    GAME_UI_SCREEN_MENU = 0,
    GAME_UI_SCREEN_GAME = 1,
    GAME_UI_SCREEN_LOAD = 2,
    GAME_UI_SCREEN_SETTINGS = 3,
    GAME_UI_SCREEN_TUTORIAL = 4,
} GameUiScreenKind;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} GameUiRect;

typedef struct {
    int viewport_width;
    int viewport_height;
    float world_origin_x;
    float world_origin_y;
    float hex_radius;
    float hex_step_x;
    float hex_step_y;
    GameHexAxial camera_center;
    GameUiRect top_bar;
    GameUiRect bottom_panel;
    GameUiRect right_panel;
    GameUiRect minimap;
    GameUiRect focus_party_button;
    GameUiRect focus_colony_button;
    GameUiRect focus_horde_button;
    GameUiRect minimap_filter_button;
    GameUiRect pause_button;
    GameUiRect speed_button;
    GameUiRect emit_noise_button;
    GameUiRect move_button;
    GameUiRect interact_button;
    GameUiRect attack_button;
    GameUiRect forensic_toggle;
    GameUiRect field_toggle;
    GameUiRect event_strip;
    GameUiRect causal_panel;
    GameUiRect main_menu_panel;
    GameUiRect menu_new_game_button;
    GameUiRect menu_load_button;
    GameUiRect menu_settings_button;
    GameUiRect menu_tutorial_button;
    GameUiRect menu_back_button;
} GameUiLayout;

typedef struct {
    GameUiHitKind kind;
    GameHexAxial tile;
} GameUiHit;

typedef struct {
    bool has_selection;
    GameUiSelectionKind selection_kind;
    GameHexAxial selected_tile;
    GameUiHit hover;
    GameHexAxial camera_center;
    GameUiActionMode active_action;
    bool paused;
    uint32_t speed_multiplier;
    bool forensic_expanded;
    bool field_overlay_visible;
    bool minimap_noise_visible;
    GameUiScreenKind screen;
    GameCommandQueueResult last_command_result;
    bool has_command_result;
} GameUiState;

void game_ui_state_init(GameUiState *ui);
void game_ui_layout_build(int viewport_width, int viewport_height, GameUiLayout *out_layout);
void game_ui_layout_set_camera(GameUiLayout *layout, GameHexAxial camera_center);
void game_ui_visible_tile_bounds(const GameUiLayout *layout, const GameDefaultScene *scene, int32_t *out_min_q,
                                 int32_t *out_max_q, int32_t *out_min_r, int32_t *out_max_r);
bool game_ui_rect_contains(GameUiRect rect, float x, float y);
bool game_ui_point_allows_camera_pan(const GameUiLayout *layout, float x, float y);
bool game_ui_hit_kind_is_world(GameUiHitKind kind);
bool game_ui_menu_hit_test(const GameUiLayout *layout, GameUiScreenKind screen, float x, float y, GameUiHit *out_hit);
bool game_ui_minimap_tile_at(const GameUiLayout *layout, const GameDefaultScene *scene, float x, float y,
                             GameHexAxial *out_tile);
void game_ui_tile_center(const GameUiLayout *layout, GameHexAxial tile, float *out_x, float *out_y);
bool game_ui_hit_test(const GameUiLayout *layout, const GameDefaultScene *scene, float x, float y, GameUiHit *out_hit);
void game_ui_handle_click(GameUiState *ui, const GameUiHit *hit);
void game_ui_handle_menu_click(GameUiState *ui, const GameUiHit *hit);
void game_ui_record_command_result(GameUiState *ui, GameCommandQueueResult result);
const char *game_ui_selection_label(GameUiSelectionKind selection);
const char *game_ui_action_label(GameUiActionMode action);
const char *game_ui_screen_label(GameUiScreenKind screen);
const char *game_ui_hit_label(GameUiHitKind hit);

#ifdef __cplusplus
}
#endif
