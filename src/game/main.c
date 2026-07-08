#include "core/log.h"
#include "game/default_scene.h"
#include "platform/app.h"
#include "render/camera.h"
#include "ui/ui_state.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef GAME_WITH_SDL3
#include "ui/sdl_hud.h"
#include <SDL3/SDL.h>
#endif

typedef struct {
    GameDefaultScene scene;
    GameUiState ui;
    uint32_t tick_count;
    int viewport_width;
    int viewport_height;
    float mouse_x;
    float mouse_y;
    bool mouse_in_window;
    bool opening_ran;
#ifdef GAME_WITH_SDL3
    GameSdlHud hud;
    GameCameraState camera;
    bool camera_ready;
    GameHexAxial last_preview_origin;
    GameHexAxial last_preview_target;
    bool has_preview_target;
    uint32_t next_preview_tick;
#endif
} BootstrapState;

#ifdef GAME_WITH_SDL3
static int32_t clamp_i32(int32_t value, int32_t min_value, int32_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static GameHexAxial bootstrap_clamp_camera_tile(const BootstrapState *state, GameHexAxial tile)
{
    if (!state) {
        return (GameHexAxial){0, 0};
    }

    tile.q = clamp_i32(tile.q, 0, state->scene.scenario.horde_anchor.q);
    tile.r = clamp_i32(tile.r, 0, state->scene.scenario.horde_anchor.r);
    return tile;
}

static void bootstrap_set_camera_center(BootstrapState *state, GameHexAxial center)
{
    if (!state) {
        return;
    }

    center = bootstrap_clamp_camera_tile(state, center);
    if (!state->camera_ready) {
        float width = state->viewport_width > 0 ? (float)state->viewport_width : 1280.0f;
        float height = state->viewport_height > 0 ? (float)state->viewport_height : 720.0f;
        if (game_camera_init(&state->camera, width, height) == GAME_CAMERA_RESULT_OK) {
            state->camera_ready = true;
        }
    }
    if (state->camera_ready) {
        state->camera.center = (GameCameraPoint){(float)center.q, (float)center.r};
        (void)game_camera_apply_leash(
            &state->camera, center,
            (float)(state->scene.scenario.horde_anchor.q + state->scene.scenario.horde_anchor.r + 1));
    }
    state->ui.camera_center = center;
}

static void bootstrap_pan_camera(BootstrapState *state, int32_t dq, int32_t dr)
{
    if (!state || (dq == 0 && dr == 0)) {
        return;
    }

    GameHexAxial center = state->ui.camera_center;
    center.q += dq;
    center.r += dr;
    bootstrap_set_camera_center(state, center);
}

static bool bootstrap_same_tile(GameHexAxial a, GameHexAxial b)
{
    return a.q == b.q && a.r == b.r;
}

static void bootstrap_configure_game_ui(BootstrapState *state)
{
    if (!state) {
        return;
    }

    state->ui.has_selection = true;
    state->ui.selection_kind = GAME_UI_SELECTION_PARTY;
    state->ui.selected_tile = state->scene.party_position;
    state->ui.camera_center = state->scene.party_position;
    state->ui.active_action = GAME_UI_ACTION_NONE;
    state->ui.hover = (GameUiHit){0};
}

static bool bootstrap_load_default_scene(BootstrapState *state)
{
    if (!state) {
        return false;
    }

    state->opening_ran = false;
    game_default_scene_shutdown(&state->scene);
    state->scene = (GameDefaultScene){0};
    if (game_default_scene_init(&state->scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_ERROR("Default scene initialization failed.");
        return false;
    }

    if (game_default_scene_run_opening(&state->scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_ERROR("Default scene opening failed.");
        return false;
    }
    if (game_default_scene_run_demo_showcase(&state->scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_ERROR("Default scene demo showcase failed.");
        return false;
    }
    bootstrap_configure_game_ui(state);
    state->opening_ran = true;
    return true;
}

static void bootstrap_start_new_game(BootstrapState *state)
{
    if (!state) {
        return;
    }
    game_ui_state_init(&state->ui);
    bootstrap_configure_game_ui(state);
    state->ui.screen = GAME_UI_SCREEN_GAME;
    state->has_preview_target = false;
    bootstrap_set_camera_center(state, state->scene.party_position);
}

static void bootstrap_regenerate_world(BootstrapState *state)
{
    if (!state) {
        return;
    }
    game_ui_state_init(&state->ui);
    if (bootstrap_load_default_scene(state)) {
        state->ui.screen = GAME_UI_SCREEN_MENU;
        state->has_preview_target = false;
        bootstrap_set_camera_center(state, state->scene.party_position);
    }
}

static void bootstrap_request_path_preview(BootstrapState *state, GameHexAxial target)
{
    if (!state) {
        return;
    }
    if (state->has_preview_target && bootstrap_same_tile(state->last_preview_origin, state->scene.party_position) &&
        bootstrap_same_tile(state->last_preview_target, target)) {
        return;
    }
    if (state->tick_count < state->next_preview_tick) {
        return;
    }

    state->last_preview_origin = state->scene.party_position;
    state->last_preview_target = target;
    state->has_preview_target = true;
    state->next_preview_tick = state->tick_count + 6u;
    (void)game_default_scene_preview_path_to(&state->scene, target);
}

static void bootstrap_select_party(BootstrapState *state)
{
    if (!state) {
        return;
    }
    state->ui.has_selection = true;
    state->ui.selection_kind = GAME_UI_SELECTION_PARTY;
    state->ui.selected_tile = state->scene.party_position;
}

static void bootstrap_issue_action(BootstrapState *state, GameUiActionMode action, GameHexAxial target)
{
    if (!state || action == GAME_UI_ACTION_NONE) {
        return;
    }

    GameCommandQueueResult command_result = GAME_COMMAND_QUEUE_RESULT_OK;
    GameDefaultSceneResult scene_result = GAME_DEFAULT_SCENE_RESULT_OK;
    switch (action) {
    case GAME_UI_ACTION_MOVE:
        scene_result = game_default_scene_move_party_to(&state->scene, target, &command_result);
        if (scene_result == GAME_DEFAULT_SCENE_RESULT_OK) {
            bootstrap_select_party(state);
        }
        break;
    case GAME_UI_ACTION_INTERACT:
        scene_result = game_default_scene_interact_at(&state->scene, target, &command_result);
        break;
    case GAME_UI_ACTION_NOISE:
        scene_result = game_default_scene_emit_noise_at(&state->scene, target, &command_result);
        break;
    case GAME_UI_ACTION_ATTACK:
        scene_result = game_default_scene_attack_at(&state->scene, target, &command_result);
        break;
    default:
        return;
    }

    game_ui_record_command_result(&state->ui, command_result);
    state->ui.active_action = GAME_UI_ACTION_NONE;
    state->has_preview_target = false;
    if (scene_result != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_WARN("%s command failed with scene result %d.", game_ui_action_label(action), (int)scene_result);
    }
}
#endif

static void bootstrap_init(void *user_data)
{
    BootstrapState *state = (BootstrapState *)user_data;
    if (!state) {
        return;
    }

#ifdef GAME_WITH_SDL3
    game_ui_state_init(&state->ui);
    if (!bootstrap_load_default_scene(state)) {
        return;
    }
    state->ui.screen = GAME_UI_SCREEN_MENU;
#else
    if (game_default_scene_init(&state->scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_ERROR("Default scene initialization failed.");
        return;
    }
    if (game_default_scene_run_opening(&state->scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_ERROR("Default scene opening failed.");
        return;
    }
    if (game_default_scene_run_demo_showcase(&state->scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_ERROR("Default scene demo showcase failed.");
        return;
    }
    game_ui_state_init(&state->ui);
    state->ui.has_selection = true;
    state->ui.selection_kind = GAME_UI_SELECTION_PARTY;
    state->ui.selected_tile = state->scene.party_position;
    state->ui.camera_center = state->scene.party_position;
    state->ui.screen = GAME_UI_SCREEN_GAME;
#endif
    state->viewport_width = 1280;
    state->viewport_height = 720;

#ifdef GAME_WITH_SDL3
    game_sdl_hud_init(&state->hud);
    if (game_camera_init(&state->camera, (float)state->viewport_width, (float)state->viewport_height) ==
        GAME_CAMERA_RESULT_OK) {
        state->camera_ready = true;
    }
    bootstrap_set_camera_center(state, state->scene.party_position);
#endif

    char summary[256] = {0};
    size_t summary_size = 0u;
    if (game_default_scene_summary(&state->scene, summary, sizeof(summary), &summary_size) ==
        GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_INFO("%s", summary);
    }
    state->opening_ran = true;
    GAME_LOG_INFO("Default scene initialized.");
}

static void bootstrap_update(void *user_data, double dt)
{
    (void)dt;
    BootstrapState *state = (BootstrapState *)user_data;
    if (!state) {
        return;
    }
    state->tick_count++;
    uint32_t updates = state->ui.paused || state->ui.screen != GAME_UI_SCREEN_GAME
                           ? 0u
                           : (state->ui.speed_multiplier == 0u ? 1u : state->ui.speed_multiplier);
    for (uint32_t i = 0u; i < updates; ++i) {
        if (state->opening_ran && game_default_scene_update(&state->scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
            GAME_LOG_WARN("Default scene update failed.");
            break;
        }
    }
    if (state->ui.selection_kind == GAME_UI_SELECTION_PARTY) {
        state->ui.selected_tile = state->scene.party_position;
    }
#ifdef GAME_WITH_SDL3
    if (state->camera_ready) {
        (void)game_camera_set_viewport(&state->camera, (float)state->viewport_width, (float)state->viewport_height);
    }
    if (state->opening_ran && state->ui.screen == GAME_UI_SCREEN_GAME && state->mouse_in_window &&
        state->viewport_width > 0 && state->viewport_height > 0 && state->tick_count % 4u == 0u) {
        GameUiLayout layout = {0};
        game_ui_layout_build(state->viewport_width, state->viewport_height, &layout);
        const float edge = 18.0f;
        const bool can_pan = game_ui_point_allows_camera_pan(&layout, state->mouse_x, state->mouse_y);
        int32_t dq = 0;
        int32_t dr = 0;
        if (can_pan && state->mouse_x <= edge) {
            dq -= 1;
        } else if (can_pan && state->mouse_x >= layout.right_panel.x - edge) {
            dq += 1;
        }
        if (can_pan && state->mouse_y <= layout.top_bar.h + edge) {
            dr -= 1;
        } else if (can_pan && state->mouse_y >= layout.bottom_panel.y - edge) {
            dr += 1;
        }
        bootstrap_pan_camera(state, dq, dr);
    }
#endif
    if (state->tick_count % 60 == 0) {
        GAME_LOG_INFO("Ticks: %u", state->tick_count);
    }
}

static void bootstrap_render(void *user_data, double interpolation)
{
    (void)interpolation;
    BootstrapState *state = (BootstrapState *)user_data;
    if (!state || !state->opening_ran) {
        return;
    }
}

#ifdef GAME_WITH_SDL3
static void bootstrap_render_sdl(void *user_data, void *renderer_ptr)
{
    BootstrapState *state = (BootstrapState *)user_data;
    SDL_Renderer *renderer = (SDL_Renderer *)renderer_ptr;
    if (!state || !state->opening_ran || !renderer) {
        return;
    }

    game_sdl_hud_render(&state->hud, renderer, &state->ui, &state->scene, state->tick_count);
}

static void bootstrap_handle_sdl_event(void *user_data, void *event_ptr, int viewport_width, int viewport_height)
{
    BootstrapState *state = (BootstrapState *)user_data;
    SDL_Event *event = (SDL_Event *)event_ptr;
    if (!state || !event || !state->opening_ran) {
        return;
    }

    state->viewport_width = viewport_width;
    state->viewport_height = viewport_height;
    if (state->camera_ready) {
        (void)game_camera_set_viewport(&state->camera, (float)viewport_width, (float)viewport_height);
    }

    if (event->type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
        state->mouse_in_window = false;
        return;
    }
    if (event->type == SDL_EVENT_WINDOW_MOUSE_ENTER) {
        state->mouse_in_window = true;
        return;
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_M) {
            state->ui.screen = GAME_UI_SCREEN_MENU;
            state->ui.active_action = GAME_UI_ACTION_NONE;
            state->ui.hover = (GameUiHit){0};
            return;
        }
        if (state->ui.screen != GAME_UI_SCREEN_GAME) {
            return;
        }
        switch (event->key.key) {
        case SDLK_LEFT:
        case SDLK_A:
            bootstrap_pan_camera(state, -1, 0);
            break;
        case SDLK_RIGHT:
        case SDLK_D:
            bootstrap_pan_camera(state, 1, 0);
            break;
        case SDLK_UP:
        case SDLK_W:
            bootstrap_pan_camera(state, 0, -1);
            break;
        case SDLK_DOWN:
        case SDLK_S:
            bootstrap_pan_camera(state, 0, 1);
            break;
        case SDLK_SPACE:
            bootstrap_set_camera_center(state, state->scene.party_position);
            break;
        case SDLK_P:
            state->ui.paused = !state->ui.paused;
            break;
        case SDLK_EQUALS:
        case SDLK_PLUS:
            state->ui.speed_multiplier = state->ui.speed_multiplier >= 4u ? 1u : state->ui.speed_multiplier * 2u;
            if (state->ui.speed_multiplier == 0u) {
                state->ui.speed_multiplier = 1u;
            }
            break;
        default:
            break;
        }
        return;
    }

    if (event->type != SDL_EVENT_MOUSE_MOTION && event->type != SDL_EVENT_MOUSE_BUTTON_DOWN) {
        return;
    }

    GameUiLayout layout = {0};
    game_ui_layout_build(viewport_width, viewport_height, &layout);
    game_ui_layout_set_camera(&layout, state->ui.camera_center);

    float x = 0.0f;
    float y = 0.0f;
    if (event->type == SDL_EVENT_MOUSE_MOTION) {
        state->mouse_in_window = true;
        state->mouse_x = event->motion.x;
        state->mouse_y = event->motion.y;
        if ((event->motion.state & SDL_BUTTON_RMASK) != 0u &&
            game_ui_point_allows_camera_pan(&layout, event->motion.x, event->motion.y)) {
            int32_t dq = 0;
            int32_t dr = 0;
            if (event->motion.xrel <= -2.0f) {
                dq = 1;
            } else if (event->motion.xrel >= 2.0f) {
                dq = -1;
            }
            if (event->motion.yrel <= -2.0f) {
                dr = 1;
            } else if (event->motion.yrel >= 2.0f) {
                dr = -1;
            }
            if (dq != 0 || dr != 0) {
                bootstrap_pan_camera(state, dq, dr);
            }
        }
        x = event->motion.x;
        y = event->motion.y;
    } else {
        state->mouse_in_window = true;
        state->mouse_x = event->button.x;
        state->mouse_y = event->button.y;
        x = event->button.x;
        y = event->button.y;
    }

    GameUiHit hit = {0};
    if (state->ui.screen != GAME_UI_SCREEN_GAME) {
        (void)game_ui_menu_hit_test(&layout, state->ui.screen, x, y, &hit);
        state->ui.hover = hit;
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
            if (hit.kind == GAME_UI_HIT_MENU_NEW_GAME) {
                bootstrap_start_new_game(state);
                return;
            }
            if (hit.kind == GAME_UI_HIT_MENU_REGENERATE) {
                bootstrap_regenerate_world(state);
                return;
            }
            game_ui_handle_menu_click(&state->ui, &hit);
        }
        return;
    }

    GameUiHit previous_hover = state->ui.hover;
    (void)game_ui_hit_test(&layout, &state->scene, x, y, &hit);
    state->ui.hover = hit;
    if (event->type == SDL_EVENT_MOUSE_MOTION && state->ui.active_action == GAME_UI_ACTION_MOVE &&
        game_ui_hit_kind_is_world(hit.kind) &&
        (previous_hover.tile.q != hit.tile.q || previous_hover.tile.r != hit.tile.r ||
         previous_hover.kind != hit.kind)) {
        bootstrap_request_path_preview(state, hit.tile);
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_RIGHT &&
        game_ui_hit_kind_is_world(hit.kind)) {
        bootstrap_issue_action(state, GAME_UI_ACTION_MOVE, hit.tile);
        return;
    }

    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        if (hit.kind == GAME_UI_HIT_MOVE_PARTY) {
            state->ui.active_action = GAME_UI_ACTION_MOVE;
            if (state->ui.has_selection) {
                bootstrap_request_path_preview(state, state->ui.selected_tile);
            }
            return;
        }
        if (hit.kind == GAME_UI_HIT_INTERACT) {
            state->ui.active_action = GAME_UI_ACTION_INTERACT;
            return;
        }
        if (hit.kind == GAME_UI_HIT_EMIT_NOISE) {
            state->ui.active_action = GAME_UI_ACTION_NOISE;
            return;
        }
        if (hit.kind == GAME_UI_HIT_ATTACK) {
            state->ui.active_action = GAME_UI_ACTION_ATTACK;
            return;
        }
        if (state->ui.active_action != GAME_UI_ACTION_NONE && game_ui_hit_kind_is_world(hit.kind)) {
            bootstrap_issue_action(state, state->ui.active_action, hit.tile);
            return;
        }
        GameHexAxial previous_camera = state->ui.camera_center;
        game_ui_handle_click(&state->ui, &hit);
        if (!bootstrap_same_tile(previous_camera, state->ui.camera_center)) {
            bootstrap_set_camera_center(state, state->ui.camera_center);
        }
    }
}
#endif

static void bootstrap_shutdown(void *user_data)
{
    BootstrapState *state = (BootstrapState *)user_data;
#ifdef GAME_WITH_SDL3
    game_sdl_hud_destroy(&state->hud);
#endif
    game_default_scene_shutdown(&state->scene);
    GAME_LOG_INFO("Bootstrap simulation shutdown after %u ticks.", state->tick_count);
}

int main(void)
{
    BootstrapState state = {0};
    GameAppConfig cfg = {
        .title = "Autonomous Simulation Horde RTS/RPG",
        .window_width = 1280,
        .window_height = 720,
        .fixed_dt = 1.0 / 60.0,
        .max_ticks = 240,
        .user_data = &state,
        .on_init = bootstrap_init,
        .on_update = bootstrap_update,
        .on_render = bootstrap_render,
        .on_shutdown = bootstrap_shutdown,
#ifdef GAME_WITH_SDL3
        .on_platform_render = bootstrap_render_sdl,
        .on_platform_event = bootstrap_handle_sdl_event,
#endif
    };

#ifdef GAME_WITH_SDL3
    cfg.max_ticks = 0;
    return game_app_run_sdl(&cfg);
#else
    return game_app_run(&cfg);
#endif
}
