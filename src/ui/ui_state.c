#include "ui/ui_state.h"

static float game_ui_absf(float value)
{
    return value < 0.0f ? -value : value;
}

static int32_t game_ui_floor_i32(float value)
{
    int32_t truncated = (int32_t)value;
    return value < (float)truncated ? truncated - 1 : truncated;
}

static int32_t game_ui_ceil_i32(float value)
{
    int32_t truncated = (int32_t)value;
    return value > (float)truncated ? truncated + 1 : truncated;
}

static int32_t game_ui_clamp_i32(int32_t value, int32_t min_value, int32_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static GameUiRect game_ui_rect(float x, float y, float w, float h)
{
    return (GameUiRect){.x = x, .y = y, .w = w, .h = h};
}

void game_ui_state_init(GameUiState *ui)
{
    if (!ui) {
        return;
    }
    *ui = (GameUiState){
        .has_selection = false,
        .selection_kind = GAME_UI_SELECTION_NONE,
        .selected_tile = {0, 0},
        .hover = {0},
        .camera_center = {0, 0},
        .active_action = GAME_UI_ACTION_NONE,
        .paused = false,
        .speed_multiplier = 1u,
        .forensic_expanded = false,
        .field_overlay_visible = false,
        .minimap_noise_visible = false,
        .screen = GAME_UI_SCREEN_MENU,
        .last_command_result = GAME_COMMAND_QUEUE_RESULT_OK,
        .has_command_result = false,
    };
}

void game_ui_layout_build(int viewport_width, int viewport_height, GameUiLayout *out_layout)
{
    if (!out_layout) {
        return;
    }

    if (viewport_width < 640) {
        viewport_width = 640;
    }
    if (viewport_height < 420) {
        viewport_height = 420;
    }

    const bool compact_actions = viewport_width < 1000;
    const float top_h = compact_actions ? 68.0f : 76.0f;
    const float bottom_h = compact_actions ? 190.0f : 184.0f;
    const float right_w = viewport_width < 1000 ? 280.0f : 340.0f;
    const float margin = compact_actions ? 14.0f : 18.0f;
    const float content_w = (float)viewport_width - right_w - margin * 3.0f;
    const float content_h = (float)viewport_height - top_h - bottom_h - margin * 2.0f;
    const float hex_radius = viewport_width < 1000 ? 9.0f : 12.0f;
    const float hex_step_x = viewport_width < 1000 ? 19.0f : 26.0f;
    const float hex_step_y = viewport_width < 1000 ? 17.0f : 22.0f;
    const float action_y = (float)viewport_height - bottom_h + (compact_actions ? 92.0f : 96.0f);
    const float minimap_w = viewport_width < 1000 ? 132.0f : 170.0f;
    const float minimap_h = bottom_h - 42.0f;
    const float action_x = margin + minimap_w + (compact_actions ? 22.0f : 34.0f);
    const float action_gap = compact_actions ? 10.0f : 14.0f;
    const float action_h = compact_actions ? 38.0f : 56.0f;
    const float action_limit = (float)viewport_width - right_w - margin;
    const float right_x = (float)viewport_width - right_w;
    float action_w = 158.0f;
    GameUiRect move_button = game_ui_rect(action_x, action_y, action_w, action_h);
    GameUiRect interact_button = game_ui_rect(action_x + action_w + action_gap, action_y, action_w, action_h);
    GameUiRect attack_button = game_ui_rect(action_x + (action_w + action_gap) * 2.0f, action_y, action_w, action_h);
    GameUiRect emit_noise_button =
        game_ui_rect(action_x + (action_w + action_gap) * 3.0f, action_y, action_w, action_h);
    if (compact_actions) {
        float available = action_limit - action_x - 4.0f;
        action_w = (available - action_gap) * 0.5f;
        if (action_w > 184.0f) {
            action_w = 184.0f;
        }
        if (action_w < 48.0f) {
            action_w = 48.0f;
        }

        move_button = game_ui_rect(action_x, action_y, action_w, action_h);
        interact_button = game_ui_rect(action_x + action_w + action_gap, action_y, action_w, action_h);
        attack_button = game_ui_rect(action_x, action_y + action_h + action_gap, action_w, action_h);
        emit_noise_button =
            game_ui_rect(action_x + action_w + action_gap, action_y + action_h + action_gap, action_w, action_h);
    }
    const float toggle_w = right_w < 300.0f ? 112.0f : 138.0f;
    const float toggle_gap = 12.0f;
    const float menu_w = viewport_width < 900 ? 430.0f : 540.0f;
    const float menu_h = 430.0f;
    const float menu_x = ((float)viewport_width - menu_w) * 0.5f;
    const float menu_y = ((float)viewport_height - menu_h) * 0.5f;
    const float menu_button_w = menu_w - 96.0f;
    const float menu_button_h = 48.0f;
    const float menu_button_x = menu_x + 48.0f;
    const float menu_button_y = menu_y + 92.0f;

    *out_layout = (GameUiLayout){
        .viewport_width = viewport_width,
        .viewport_height = viewport_height,
        .world_origin_x = margin + content_w * 0.50f,
        .world_origin_y = top_h + margin + content_h * 0.50f,
        .hex_radius = hex_radius,
        .hex_step_x = hex_step_x,
        .hex_step_y = hex_step_y,
        .camera_center = {0, 0},
        .top_bar = game_ui_rect(0.0f, 0.0f, (float)viewport_width, top_h),
        .bottom_panel = game_ui_rect(0.0f, (float)viewport_height - bottom_h, (float)viewport_width, bottom_h),
        .right_panel = game_ui_rect(right_x, top_h, right_w, (float)viewport_height - top_h - bottom_h),
        .minimap = game_ui_rect(margin, (float)viewport_height - bottom_h + 22.0f, minimap_w, minimap_h),
        .focus_party_button =
            game_ui_rect(margin + minimap_w + 14.0f, (float)viewport_height - bottom_h + 22.0f, 34.0f, 34.0f),
        .focus_colony_button =
            game_ui_rect(margin + minimap_w + 54.0f, (float)viewport_height - bottom_h + 22.0f, 34.0f, 34.0f),
        .focus_horde_button =
            game_ui_rect(margin + minimap_w + 94.0f, (float)viewport_height - bottom_h + 22.0f, 34.0f, 34.0f),
        .minimap_filter_button =
            game_ui_rect(margin + minimap_w + 134.0f, (float)viewport_height - bottom_h + 22.0f, 34.0f, 34.0f),
        .pause_button = game_ui_rect((float)viewport_width - right_w - 176.0f, 18.0f, 76.0f, 40.0f),
        .speed_button = game_ui_rect((float)viewport_width - right_w - 90.0f, 18.0f, 72.0f, 40.0f),
        .move_button = move_button,
        .interact_button = interact_button,
        .attack_button = attack_button,
        .emit_noise_button = emit_noise_button,
        .forensic_toggle = game_ui_rect(right_x + 22.0f, top_h + 250.0f, toggle_w, 38.0f),
        .field_toggle = game_ui_rect(right_x + 22.0f + toggle_w + toggle_gap, top_h + 250.0f, toggle_w, 38.0f),
        .event_strip = game_ui_rect(margin, top_h + 14.0f, 360.0f, 50.0f),
        .causal_panel = game_ui_rect(right_x + 22.0f, top_h + 304.0f, right_w - 44.0f, 134.0f),
        .main_menu_panel = game_ui_rect(menu_x, menu_y, menu_w, menu_h),
        .menu_new_game_button = game_ui_rect(menu_button_x, menu_button_y, menu_button_w, menu_button_h),
        .menu_regenerate_button = game_ui_rect(menu_button_x, menu_button_y + 56.0f, menu_button_w, menu_button_h),
        .menu_load_button = game_ui_rect(menu_button_x, menu_button_y + 112.0f, menu_button_w, menu_button_h),
        .menu_settings_button = game_ui_rect(menu_button_x, menu_button_y + 168.0f, menu_button_w, menu_button_h),
        .menu_tutorial_button = game_ui_rect(menu_button_x, menu_button_y + 224.0f, menu_button_w, menu_button_h),
        .menu_back_button = game_ui_rect(menu_button_x, menu_y + menu_h - 82.0f, menu_button_w, 52.0f),
    };
}

void game_ui_layout_set_camera(GameUiLayout *layout, GameHexAxial camera_center)
{
    if (!layout) {
        return;
    }
    layout->camera_center = camera_center;
}

void game_ui_visible_tile_bounds(const GameUiLayout *layout, const GameDefaultScene *scene, int32_t *out_min_q,
                                 int32_t *out_max_q, int32_t *out_min_r, int32_t *out_max_r)
{
    int32_t min_q = 0;
    int32_t max_q = 0;
    int32_t min_r = 0;
    int32_t max_r = 0;
    if (layout && scene && layout->hex_step_x > 0.0f && layout->hex_step_y > 0.0f) {
        const int32_t map_max_q = scene->scenario.horde_anchor.q;
        const int32_t map_max_r = scene->scenario.horde_anchor.r;
        const float top_y = layout->top_bar.h - layout->hex_step_y;
        const float bottom_y = layout->bottom_panel.y + layout->hex_step_y;
        const float left_x = -layout->hex_step_x;
        const float right_x = layout->right_panel.x + layout->hex_step_x;

        min_r = layout->camera_center.r + game_ui_floor_i32((top_y - layout->world_origin_y) / layout->hex_step_y) - 2;
        max_r =
            layout->camera_center.r + game_ui_ceil_i32((bottom_y - layout->world_origin_y) / layout->hex_step_y) + 2;
        min_r = game_ui_clamp_i32(min_r, 0, map_max_r);
        max_r = game_ui_clamp_i32(max_r, 0, map_max_r);

        min_q = map_max_q;
        max_q = 0;
        for (int32_t r = min_r; r <= max_r; ++r) {
            float rel_r = (float)(r - layout->camera_center.r);
            int32_t row_min_q =
                layout->camera_center.q +
                game_ui_floor_i32((left_x - layout->world_origin_x) / layout->hex_step_x - rel_r * 0.46f) - 2;
            int32_t row_max_q =
                layout->camera_center.q +
                game_ui_ceil_i32((right_x - layout->world_origin_x) / layout->hex_step_x - rel_r * 0.46f) + 2;
            row_min_q = game_ui_clamp_i32(row_min_q, 0, map_max_q);
            row_max_q = game_ui_clamp_i32(row_max_q, 0, map_max_q);
            if (row_min_q < min_q) {
                min_q = row_min_q;
            }
            if (row_max_q > max_q) {
                max_q = row_max_q;
            }
        }
    }

    if (out_min_q) {
        *out_min_q = min_q;
    }
    if (out_max_q) {
        *out_max_q = max_q;
    }
    if (out_min_r) {
        *out_min_r = min_r;
    }
    if (out_max_r) {
        *out_max_r = max_r;
    }
}

bool game_ui_rect_contains(GameUiRect rect, float x, float y)
{
    return x >= rect.x && y >= rect.y && x <= rect.x + rect.w && y <= rect.y + rect.h;
}

bool game_ui_point_allows_camera_pan(const GameUiLayout *layout, float x, float y)
{
    if (!layout) {
        return false;
    }

    if (x < 0.0f || y < layout->top_bar.h || x >= layout->right_panel.x || y >= layout->bottom_panel.y) {
        return false;
    }
    if (game_ui_rect_contains(layout->event_strip, x, y)) {
        return false;
    }
    return true;
}

bool game_ui_hit_kind_is_world(GameUiHitKind kind)
{
    return kind == GAME_UI_HIT_TILE || kind == GAME_UI_HIT_PARTY || kind == GAME_UI_HIT_COLONY ||
           kind == GAME_UI_HIT_HORDE || kind == GAME_UI_HIT_WORKER;
}

bool game_ui_menu_hit_test(const GameUiLayout *layout, GameUiScreenKind screen, float x, float y, GameUiHit *out_hit)
{
    if (!layout || !out_hit) {
        return false;
    }

    *out_hit = (GameUiHit){.kind = GAME_UI_HIT_NONE, .tile = {0, 0}};
    if (screen == GAME_UI_SCREEN_MENU) {
        if (game_ui_rect_contains(layout->menu_new_game_button, x, y)) {
            *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MENU_NEW_GAME, .tile = {0, 0}};
            return true;
        }
        if (game_ui_rect_contains(layout->menu_regenerate_button, x, y)) {
            *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MENU_REGENERATE, .tile = {0, 0}};
            return true;
        }
        if (game_ui_rect_contains(layout->menu_load_button, x, y)) {
            *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MENU_LOAD, .tile = {0, 0}};
            return true;
        }
        if (game_ui_rect_contains(layout->menu_settings_button, x, y)) {
            *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MENU_SETTINGS, .tile = {0, 0}};
            return true;
        }
        if (game_ui_rect_contains(layout->menu_tutorial_button, x, y)) {
            *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MENU_TUTORIAL, .tile = {0, 0}};
            return true;
        }
    }
    if (game_ui_rect_contains(layout->menu_back_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MENU_BACK, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->main_menu_panel, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_CHROME, .tile = {0, 0}};
        return true;
    }
    return false;
}

bool game_ui_minimap_tile_at(const GameUiLayout *layout, const GameDefaultScene *scene, float x, float y,
                             GameHexAxial *out_tile)
{
    if (!layout || !scene || !out_tile || !game_ui_rect_contains(layout->minimap, x, y)) {
        return false;
    }

    float nx = (x - layout->minimap.x) / layout->minimap.w;
    float ny = (y - layout->minimap.y) / layout->minimap.h;
    if (nx < 0.0f) {
        nx = 0.0f;
    }
    if (ny < 0.0f) {
        ny = 0.0f;
    }
    if (nx > 1.0f) {
        nx = 1.0f;
    }
    if (ny > 1.0f) {
        ny = 1.0f;
    }

    int32_t max_q = scene->scenario.horde_anchor.q;
    int32_t max_r = scene->scenario.horde_anchor.r;
    out_tile->q = (int32_t)(nx * (float)max_q + 0.5f);
    out_tile->r = (int32_t)(ny * (float)max_r + 0.5f);
    if (out_tile->q < 0) {
        out_tile->q = 0;
    }
    if (out_tile->r < 0) {
        out_tile->r = 0;
    }
    if (out_tile->q > max_q) {
        out_tile->q = max_q;
    }
    if (out_tile->r > max_r) {
        out_tile->r = max_r;
    }
    return true;
}

void game_ui_minimap_sample_stride(const GameUiLayout *layout, const GameDefaultScene *scene, int32_t *out_q_step,
                                   int32_t *out_r_step)
{
    int32_t q_step = 1;
    int32_t r_step = 1;
    if (layout && scene && layout->minimap.w > 0.0f && layout->minimap.h > 0.0f) {
        int32_t q_tiles = scene->scenario.horde_anchor.q + 1;
        int32_t r_tiles = scene->scenario.horde_anchor.r + 1;
        int32_t pixel_w = layout->minimap.w > 1.0f ? (int32_t)layout->minimap.w : 1;
        int32_t pixel_h = layout->minimap.h > 1.0f ? (int32_t)layout->minimap.h : 1;
        if (q_tiles > pixel_w) {
            q_step = (q_tiles + pixel_w - 1) / pixel_w;
        }
        if (r_tiles > pixel_h) {
            r_step = (r_tiles + pixel_h - 1) / pixel_h;
        }
    }
    if (q_step < 1) {
        q_step = 1;
    }
    if (r_step < 1) {
        r_step = 1;
    }
    if (out_q_step) {
        *out_q_step = q_step;
    }
    if (out_r_step) {
        *out_r_step = r_step;
    }
}

void game_ui_tile_center(const GameUiLayout *layout, GameHexAxial tile, float *out_x, float *out_y)
{
    if (!layout) {
        if (out_x) {
            *out_x = 0.0f;
        }
        if (out_y) {
            *out_y = 0.0f;
        }
        return;
    }

    if (out_x) {
        int32_t rel_q = tile.q - layout->camera_center.q;
        int32_t rel_r = tile.r - layout->camera_center.r;
        *out_x = layout->world_origin_x + (float)rel_q * layout->hex_step_x + (float)rel_r * layout->hex_step_x * 0.46f;
    }
    if (out_y) {
        int32_t rel_r = tile.r - layout->camera_center.r;
        *out_y = layout->world_origin_y + (float)rel_r * layout->hex_step_y;
    }
}

static bool game_ui_marker_hit(const GameUiLayout *layout, GameHexAxial marker, GameUiHitKind kind, float x, float y,
                               GameUiHit *out_hit)
{
    float cx = 0.0f;
    float cy = 0.0f;
    game_ui_tile_center(layout, marker, &cx, &cy);
    if (game_ui_absf(x - cx) <= layout->hex_radius * 0.75f && game_ui_absf(y - cy) <= layout->hex_radius * 0.75f) {
        if (out_hit) {
            *out_hit = (GameUiHit){.kind = kind, .tile = marker};
        }
        return true;
    }
    return false;
}

static bool game_ui_tile_hit(const GameUiLayout *layout, const GameDefaultScene *scene, float x, float y,
                             GameUiHit *out_hit)
{
    if (!layout || !scene) {
        return false;
    }

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
            float nx = game_ui_absf(x - cx) / layout->hex_radius;
            float ny = game_ui_absf(y - cy) / (layout->hex_radius * 0.86f);
            if (nx + ny * 0.55f <= 1.15f) {
                if (out_hit) {
                    *out_hit = (GameUiHit){.kind = GAME_UI_HIT_TILE, .tile = tile};
                }
                return true;
            }
        }
    }
    return false;
}

bool game_ui_hit_test(const GameUiLayout *layout, const GameDefaultScene *scene, float x, float y, GameUiHit *out_hit)
{
    if (!layout || !scene || !out_hit) {
        return false;
    }

    *out_hit = (GameUiHit){0};
    GameHexAxial minimap_tile = {0, 0};
    if (game_ui_minimap_tile_at(layout, scene, x, y, &minimap_tile)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MINIMAP, .tile = minimap_tile};
        return true;
    }
    if (game_ui_rect_contains(layout->pause_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_PAUSE_TOGGLE, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->speed_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_SPEED_TOGGLE, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->focus_party_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_FOCUS_PARTY, .tile = scene->party_position};
        return true;
    }
    if (game_ui_rect_contains(layout->focus_colony_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_FOCUS_COLONY, .tile = scene->scenario.colony_anchor};
        return true;
    }
    if (game_ui_rect_contains(layout->focus_horde_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_FOCUS_HORDE, .tile = scene->scenario.horde_anchor};
        return true;
    }
    if (game_ui_rect_contains(layout->minimap_filter_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MINIMAP_FILTER, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->move_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_MOVE_PARTY, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->interact_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_INTERACT, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->attack_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_ATTACK, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->emit_noise_button, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_EMIT_NOISE, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->forensic_toggle, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_FORENSIC_TOGGLE, .tile = {0, 0}};
        return true;
    }
    if (game_ui_rect_contains(layout->field_toggle, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_FIELD_TOGGLE, .tile = {0, 0}};
        return true;
    }
    if (!game_ui_point_allows_camera_pan(layout, x, y)) {
        *out_hit = (GameUiHit){.kind = GAME_UI_HIT_CHROME, .tile = {0, 0}};
        return true;
    }

    if (game_ui_marker_hit(layout, scene->party_position, GAME_UI_HIT_PARTY, x, y, out_hit) ||
        game_ui_marker_hit(layout, scene->scenario.colony_anchor, GAME_UI_HIT_COLONY, x, y, out_hit) ||
        game_ui_marker_hit(layout, scene->scenario.horde_anchor, GAME_UI_HIT_HORDE, x, y, out_hit) ||
        game_ui_marker_hit(layout, scene->worker.position, GAME_UI_HIT_WORKER, x, y, out_hit)) {
        return true;
    }

    return game_ui_tile_hit(layout, scene, x, y, out_hit);
}

void game_ui_handle_click(GameUiState *ui, const GameUiHit *hit)
{
    if (!ui || !hit) {
        return;
    }

    switch (hit->kind) {
    case GAME_UI_HIT_PARTY:
        ui->has_selection = true;
        ui->selection_kind = GAME_UI_SELECTION_PARTY;
        ui->selected_tile = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    case GAME_UI_HIT_COLONY:
        ui->has_selection = true;
        ui->selection_kind = GAME_UI_SELECTION_COLONY;
        ui->selected_tile = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    case GAME_UI_HIT_HORDE:
        ui->has_selection = true;
        ui->selection_kind = GAME_UI_SELECTION_HORDE;
        ui->selected_tile = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    case GAME_UI_HIT_WORKER:
        ui->has_selection = true;
        ui->selection_kind = GAME_UI_SELECTION_WORKER;
        ui->selected_tile = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    case GAME_UI_HIT_TILE:
        ui->has_selection = true;
        ui->selection_kind = GAME_UI_SELECTION_TILE;
        ui->selected_tile = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    case GAME_UI_HIT_MOVE_PARTY:
        ui->active_action = GAME_UI_ACTION_MOVE;
        break;
    case GAME_UI_HIT_INTERACT:
        ui->active_action = GAME_UI_ACTION_INTERACT;
        break;
    case GAME_UI_HIT_EMIT_NOISE:
        ui->active_action = GAME_UI_ACTION_NOISE;
        break;
    case GAME_UI_HIT_ATTACK:
        ui->active_action = GAME_UI_ACTION_ATTACK;
        break;
    case GAME_UI_HIT_PAUSE_TOGGLE:
        ui->paused = !ui->paused;
        break;
    case GAME_UI_HIT_SPEED_TOGGLE:
        ui->speed_multiplier = ui->speed_multiplier >= 4u ? 1u : ui->speed_multiplier * 2u;
        if (ui->speed_multiplier == 0u) {
            ui->speed_multiplier = 1u;
        }
        break;
    case GAME_UI_HIT_MINIMAP_FILTER:
        ui->minimap_noise_visible = !ui->minimap_noise_visible;
        break;
    case GAME_UI_HIT_FORENSIC_TOGGLE:
        ui->forensic_expanded = !ui->forensic_expanded;
        break;
    case GAME_UI_HIT_FIELD_TOGGLE:
        ui->field_overlay_visible = !ui->field_overlay_visible;
        break;
    case GAME_UI_HIT_MINIMAP:
        ui->camera_center = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    case GAME_UI_HIT_FOCUS_PARTY:
        ui->camera_center = hit->tile;
        ui->has_selection = true;
        ui->selection_kind = GAME_UI_SELECTION_PARTY;
        ui->selected_tile = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    case GAME_UI_HIT_FOCUS_COLONY:
        ui->camera_center = hit->tile;
        ui->has_selection = true;
        ui->selection_kind = GAME_UI_SELECTION_COLONY;
        ui->selected_tile = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    case GAME_UI_HIT_FOCUS_HORDE:
        ui->camera_center = hit->tile;
        ui->has_selection = true;
        ui->selection_kind = GAME_UI_SELECTION_HORDE;
        ui->selected_tile = hit->tile;
        ui->active_action = GAME_UI_ACTION_NONE;
        break;
    default:
        break;
    }
}

void game_ui_handle_menu_click(GameUiState *ui, const GameUiHit *hit)
{
    if (!ui || !hit) {
        return;
    }

    switch (hit->kind) {
    case GAME_UI_HIT_MENU_NEW_GAME:
        ui->screen = GAME_UI_SCREEN_GAME;
        break;
    case GAME_UI_HIT_MENU_REGENERATE:
        break;
    case GAME_UI_HIT_MENU_LOAD:
        ui->screen = GAME_UI_SCREEN_LOAD;
        break;
    case GAME_UI_HIT_MENU_SETTINGS:
        ui->screen = GAME_UI_SCREEN_SETTINGS;
        break;
    case GAME_UI_HIT_MENU_TUTORIAL:
        ui->screen = GAME_UI_SCREEN_TUTORIAL;
        break;
    case GAME_UI_HIT_MENU_BACK:
        ui->screen = GAME_UI_SCREEN_MENU;
        break;
    default:
        break;
    }
}

void game_ui_record_command_result(GameUiState *ui, GameCommandQueueResult result)
{
    if (!ui) {
        return;
    }
    ui->last_command_result = result;
    ui->has_command_result = true;
}

const char *game_ui_selection_label(GameUiSelectionKind selection)
{
    switch (selection) {
    case GAME_UI_SELECTION_PARTY:
        return "Party";
    case GAME_UI_SELECTION_COLONY:
        return "Colony";
    case GAME_UI_SELECTION_HORDE:
        return "Horde";
    case GAME_UI_SELECTION_WORKER:
        return "Worker";
    case GAME_UI_SELECTION_TILE:
        return "Tile";
    default:
        return "None";
    }
}

const char *game_ui_action_label(GameUiActionMode action)
{
    switch (action) {
    case GAME_UI_ACTION_MOVE:
        return "Move";
    case GAME_UI_ACTION_INTERACT:
        return "Interact";
    case GAME_UI_ACTION_NOISE:
        return "Noise";
    case GAME_UI_ACTION_ATTACK:
        return "Attack";
    default:
        return "None";
    }
}

const char *game_ui_screen_label(GameUiScreenKind screen)
{
    switch (screen) {
    case GAME_UI_SCREEN_GAME:
        return "Game";
    case GAME_UI_SCREEN_LOAD:
        return "Load";
    case GAME_UI_SCREEN_SETTINGS:
        return "Settings";
    case GAME_UI_SCREEN_TUTORIAL:
        return "Tutorial";
    default:
        return "Menu";
    }
}

const char *game_ui_hit_label(GameUiHitKind hit)
{
    switch (hit) {
    case GAME_UI_HIT_TILE:
        return "Tile";
    case GAME_UI_HIT_PARTY:
        return "Party";
    case GAME_UI_HIT_COLONY:
        return "Colony";
    case GAME_UI_HIT_HORDE:
        return "Horde";
    case GAME_UI_HIT_WORKER:
        return "Worker";
    case GAME_UI_HIT_EMIT_NOISE:
        return "Noise action";
    case GAME_UI_HIT_MOVE_PARTY:
        return "Move action";
    case GAME_UI_HIT_INTERACT:
        return "Interact action";
    case GAME_UI_HIT_ATTACK:
        return "Attack action";
    case GAME_UI_HIT_MINIMAP:
        return "Minimap";
    case GAME_UI_HIT_FOCUS_PARTY:
        return "Focus party";
    case GAME_UI_HIT_FOCUS_COLONY:
        return "Focus colony";
    case GAME_UI_HIT_FOCUS_HORDE:
        return "Focus horde";
    case GAME_UI_HIT_PAUSE_TOGGLE:
        return "Pause";
    case GAME_UI_HIT_SPEED_TOGGLE:
        return "Speed";
    case GAME_UI_HIT_MINIMAP_FILTER:
        return "Minimap filter";
    case GAME_UI_HIT_FORENSIC_TOGGLE:
        return "Causal panel";
    case GAME_UI_HIT_FIELD_TOGGLE:
        return "Field overlay";
    case GAME_UI_HIT_CHROME:
        return "UI";
    case GAME_UI_HIT_MENU_NEW_GAME:
        return "New game";
    case GAME_UI_HIT_MENU_REGENERATE:
        return "Regenerate world";
    case GAME_UI_HIT_MENU_LOAD:
        return "Load";
    case GAME_UI_HIT_MENU_SETTINGS:
        return "Settings";
    case GAME_UI_HIT_MENU_TUTORIAL:
        return "Tutorial";
    case GAME_UI_HIT_MENU_BACK:
        return "Back";
    default:
        return "None";
    }
}
