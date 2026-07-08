#include <stdio.h>

#include "ui/ui_state.h"

static int assert_true(int condition, const char *label)
{
    if (!condition) {
        printf("[ui_state] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int assert_rect_before(GameUiRect rect, float limit, const char *label)
{
    return assert_true(rect.x >= 0.0f && rect.x + rect.w <= limit, label);
}

static int test_ui_layout_stability(void)
{
    int failed = 0;
    GameUiLayout desktop = {0};
    GameUiLayout compact = {0};
    game_ui_layout_build(1280, 720, &desktop);
    game_ui_layout_build(800, 480, &compact);

    failed += assert_true(desktop.right_panel.w == 340.0f, "desktop right panel width");
    failed += assert_true(compact.right_panel.w == 280.0f, "compact right panel width");
    failed += assert_true(desktop.bottom_panel.h == 184.0f, "desktop bottom panel height");
    failed += assert_true(compact.bottom_panel.h == 190.0f, "compact bottom panel reserves action grid");
    failed += assert_true(desktop.hex_radius >= 12.0f, "desktop hex radius readable");
    failed += assert_true(compact.hex_radius >= 9.0f, "compact hex radius readable");
    failed += assert_true(desktop.minimap.w > 0.0f && desktop.minimap.h > 0.0f, "desktop minimap present");
    failed += assert_true(desktop.minimap.x < desktop.move_button.x, "minimap precedes action bar");
    failed += assert_true(desktop.move_button.x < desktop.interact_button.x, "move button precedes interact");
    failed += assert_true(desktop.interact_button.x < desktop.attack_button.x, "interact button precedes attack");
    failed += assert_true(desktop.attack_button.x < desktop.emit_noise_button.x, "attack button precedes noise");
    failed += assert_true(desktop.pause_button.y < desktop.bottom_panel.y, "pause button in top bar");
    float compact_limit = compact.right_panel.x - 16.0f;
    failed += assert_rect_before(compact.move_button, compact_limit, "compact move button before inspector");
    failed += assert_rect_before(compact.interact_button, compact_limit, "compact interact button before inspector");
    failed += assert_rect_before(compact.attack_button, compact_limit, "compact attack button before inspector");
    failed += assert_rect_before(compact.emit_noise_button, compact_limit, "compact noise button before inspector");
    failed += assert_true(compact.attack_button.y > compact.move_button.y, "compact actions wrap to second row");
    failed += assert_true(compact.field_toggle.x + compact.field_toggle.w <= compact.viewport_width,
                          "compact field toggle stays visible");
    failed += assert_true(desktop.menu_new_game_button.h >= 48.0f, "menu buttons are large");
    failed += assert_true(desktop.menu_new_game_button.y < desktop.menu_regenerate_button.y,
                          "regenerate follows enter world");
    failed += assert_true(game_ui_rect_contains(desktop.emit_noise_button, desktop.emit_noise_button.x + 1.0f,
                                                desktop.emit_noise_button.y + 1.0f),
                          "emit button hit rect stable");
    failed += assert_true(!game_ui_point_allows_camera_pan(&desktop, 12.0f, desktop.top_bar.y + 12.0f),
                          "top bar blocks camera pan");
    failed += assert_true(
        !game_ui_point_allows_camera_pan(&desktop, desktop.right_panel.x + 12.0f, desktop.right_panel.y + 12.0f),
        "right panel blocks camera pan");
    failed += assert_true(!game_ui_point_allows_camera_pan(&desktop, 12.0f, desktop.bottom_panel.y + 12.0f),
                          "bottom panel blocks camera pan");
    failed += assert_true(
        !game_ui_point_allows_camera_pan(&desktop, desktop.event_strip.x + 2.0f, desktop.event_strip.y + 2.0f),
        "event strip blocks camera pan");
    failed += assert_true(game_ui_point_allows_camera_pan(&desktop, 12.0f, desktop.top_bar.h + 12.0f),
                          "world edge allows camera pan");
    GameDefaultScene scene = {0};
    failed +=
        assert_true(game_default_scene_init(&scene) == GAME_DEFAULT_SCENE_RESULT_OK, "scene init for visible bounds");
    game_ui_layout_set_camera(&desktop, scene.party_position);
    int32_t min_q = 0;
    int32_t max_q = 0;
    int32_t min_r = 0;
    int32_t max_r = 0;
    game_ui_visible_tile_bounds(&desktop, &scene, &min_q, &max_q, &min_r, &max_r);
    int32_t visible_area = (max_q - min_q + 1) * (max_r - min_r + 1);
    int32_t map_area = (scene.scenario.horde_anchor.q + 1) * (scene.scenario.horde_anchor.r + 1);
    failed += assert_true(visible_area > 0 && visible_area < map_area, "visible bounds avoid full-map scan");
    int32_t q_step = 0;
    int32_t r_step = 0;
    game_ui_minimap_sample_stride(&desktop, &scene, &q_step, &r_step);
    int32_t sampled_area = ((scene.scenario.horde_anchor.q + q_step) / q_step) *
                           ((scene.scenario.horde_anchor.r + r_step) / r_step);
    int32_t minimap_pixels = (int32_t)(desktop.minimap.w * desktop.minimap.h);
    failed += assert_true(q_step > 1 && r_step > 1, "large minimap uses sampled stride");
    failed += assert_true(sampled_area <= minimap_pixels + 512, "minimap sampling is pixel bounded");
    game_default_scene_shutdown(&scene);
    return failed;
}

static int test_ui_hit_testing_and_selection(void)
{
    int failed = 0;
    GameDefaultScene scene = {0};
    failed += assert_true(game_default_scene_init(&scene) == GAME_DEFAULT_SCENE_RESULT_OK, "scene init");

    GameUiLayout layout = {0};
    game_ui_layout_build(1280, 720, &layout);
    game_ui_layout_set_camera(&layout, scene.party_position);
    float hx = 0.0f;
    float hy = 0.0f;
    game_ui_tile_center(&layout, scene.party_position, &hx, &hy);
    failed += assert_true(hx < layout.right_panel.x, "camera keeps party before inspector");
    failed += assert_true(hy < layout.bottom_panel.y, "camera keeps party above action panel");
    GameUiState ui = {0};
    game_ui_state_init(&ui);
    failed += assert_true(ui.screen == GAME_UI_SCREEN_MENU, "ui starts at main menu");
    failed += assert_true(game_ui_screen_label(ui.screen)[0] != '\0', "screen labels available");

    float x = 0.0f;
    float y = 0.0f;
    game_ui_tile_center(&layout, scene.scenario.party_anchor, &x, &y);

    GameUiHit hit = {0};
    GameHexAxial minimap_tile = {0, 0};
    failed += assert_true(game_ui_minimap_tile_at(&layout, &scene, layout.minimap.x + layout.minimap.w,
                                                  layout.minimap.y + layout.minimap.h, &minimap_tile),
                          "minimap tile maps");
    failed += assert_true(minimap_tile.q == scene.scenario.horde_anchor.q, "minimap maps max q");
    failed += assert_true(minimap_tile.r == scene.scenario.horde_anchor.r, "minimap maps max r");
    failed += assert_true(game_ui_hit_test(&layout, &scene, layout.minimap.x + layout.minimap.w,
                                           layout.minimap.y + layout.minimap.h, &hit),
                          "minimap hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_MINIMAP, "minimap hit classified");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.camera_center.q == scene.scenario.horde_anchor.q, "minimap click jumps camera q");
    failed += assert_true(ui.camera_center.r == scene.scenario.horde_anchor.r, "minimap click jumps camera r");
    game_ui_layout_set_camera(&layout, scene.party_position);
    ui.camera_center = scene.party_position;

    failed += assert_true(game_ui_hit_test(&layout, &scene, x, y, &hit), "party marker hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_PARTY, "marker priority over tile");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.has_selection && ui.selection_kind == GAME_UI_SELECTION_PARTY, "party selected");

    GameHexAxial plain_tile = {1, 0};
    game_ui_tile_center(&layout, plain_tile, &x, &y);
    failed += assert_true(game_ui_hit_test(&layout, &scene, x, y, &hit), "tile hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_TILE, "plain tile classified");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.selection_kind == GAME_UI_SELECTION_TILE && ui.selected_tile.q == 1, "tile selected");

    failed += assert_true(
        game_ui_hit_test(&layout, &scene, layout.emit_noise_button.x + 4.0f, layout.emit_noise_button.y + 4.0f, &hit),
        "button hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_EMIT_NOISE, "button classified");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.active_action == GAME_UI_ACTION_NOISE, "noise button stages action");
    failed +=
        assert_true(game_ui_hit_test(&layout, &scene, layout.move_button.x + 4.0f, layout.move_button.y + 4.0f, &hit),
                    "move button hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_MOVE_PARTY, "move button classified");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.active_action == GAME_UI_ACTION_MOVE, "move button stages action");
    failed += assert_true(
        game_ui_hit_test(&layout, &scene, layout.interact_button.x + 4.0f, layout.interact_button.y + 4.0f, &hit),
        "interact button hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_INTERACT, "interact button classified");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.active_action == GAME_UI_ACTION_INTERACT, "interact button stages action");
    failed += assert_true(
        game_ui_hit_test(&layout, &scene, layout.attack_button.x + 4.0f, layout.attack_button.y + 4.0f, &hit),
        "attack button hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_ATTACK, "attack button classified");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.active_action == GAME_UI_ACTION_ATTACK, "attack button stages action");
    failed += assert_true(game_ui_hit_label(hit.kind)[0] != '\0', "hit labels support tooltips");
    game_ui_handle_click(&ui, &(GameUiHit){.kind = GAME_UI_HIT_TILE, .tile = plain_tile});
    failed += assert_true(ui.active_action == GAME_UI_ACTION_NONE, "world selection clears staged action");

    failed +=
        assert_true(game_ui_hit_test(&layout, &scene, layout.pause_button.x + 2.0f, layout.pause_button.y + 2.0f, &hit),
                    "pause hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_PAUSE_TOGGLE, "pause classified");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.paused, "pause toggles");
    failed +=
        assert_true(game_ui_hit_test(&layout, &scene, layout.speed_button.x + 2.0f, layout.speed_button.y + 2.0f, &hit),
                    "speed hit");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.speed_multiplier == 2u, "speed cycles to 2x");
    failed += assert_true(game_ui_hit_test(&layout, &scene, layout.minimap_filter_button.x + 2.0f,
                                           layout.minimap_filter_button.y + 2.0f, &hit),
                          "minimap filter hit");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.minimap_noise_visible, "minimap filter toggles noise");

    failed += assert_true(game_ui_hit_test(&layout, &scene, layout.focus_colony_button.x + 2.0f,
                                           layout.focus_colony_button.y + 2.0f, &hit),
                          "colony focus hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_FOCUS_COLONY, "colony focus classified");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.camera_center.q == scene.scenario.colony_anchor.q, "colony focus jumps camera");
    failed += assert_true(ui.selection_kind == GAME_UI_SELECTION_COLONY, "colony focus selects colony");

    failed += assert_true(!ui.forensic_expanded, "forensic compact by default");
    failed += assert_true(
        game_ui_hit_test(&layout, &scene, layout.forensic_toggle.x + 2.0f, layout.forensic_toggle.y + 2.0f, &hit),
        "forensic toggle hit");
    game_ui_handle_click(&ui, &hit);
    failed += assert_true(ui.forensic_expanded, "forensic toggles open");

    failed +=
        assert_true(game_ui_hit_test(&layout, &scene, layout.right_panel.x + 12.0f, layout.right_panel.y + 12.0f, &hit),
                    "empty inspector chrome hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_CHROME, "empty inspector does not hit world");
    failed +=
        assert_true(game_ui_hit_test(&layout, &scene, layout.event_strip.x + 2.0f, layout.event_strip.y + 2.0f, &hit),
                    "event strip chrome hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_CHROME, "event strip does not hit world");

    failed += assert_true(game_ui_menu_hit_test(&layout, ui.screen, layout.menu_new_game_button.x + 4.0f,
                                                layout.menu_new_game_button.y + 4.0f, &hit),
                          "new game menu hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_MENU_NEW_GAME, "new game classified");
    game_ui_handle_menu_click(&ui, &hit);
    failed += assert_true(ui.screen == GAME_UI_SCREEN_GAME, "new game opens game screen");
    ui.screen = GAME_UI_SCREEN_MENU;
    failed += assert_true(game_ui_menu_hit_test(&layout, ui.screen, layout.menu_regenerate_button.x + 4.0f,
                                                layout.menu_regenerate_button.y + 4.0f, &hit),
                          "regenerate menu hit");
    failed += assert_true(hit.kind == GAME_UI_HIT_MENU_REGENERATE, "regenerate classified");
    game_ui_handle_menu_click(&ui, &hit);
    failed += assert_true(ui.screen == GAME_UI_SCREEN_MENU, "regenerate hit stays on menu for app handler");
    failed += assert_true(game_ui_hit_label(hit.kind)[0] != '\0', "regenerate hit label available");
    failed += assert_true(game_ui_menu_hit_test(&layout, ui.screen, layout.menu_tutorial_button.x + 4.0f,
                                                layout.menu_tutorial_button.y + 4.0f, &hit),
                          "tutorial menu hit");
    game_ui_handle_menu_click(&ui, &hit);
    failed += assert_true(ui.screen == GAME_UI_SCREEN_TUTORIAL, "tutorial opens tutorial screen");
    failed += assert_true(!game_ui_menu_hit_test(&layout, ui.screen, layout.menu_new_game_button.x + 4.0f,
                                                 layout.menu_new_game_button.y + 4.0f, &hit) ||
                              hit.kind != GAME_UI_HIT_MENU_NEW_GAME,
                          "subscreen ignores hidden main-menu buttons");
    failed += assert_true(game_ui_menu_hit_test(&layout, ui.screen, layout.menu_back_button.x + 4.0f,
                                                layout.menu_back_button.y + 4.0f, &hit),
                          "back menu hit");
    game_ui_handle_menu_click(&ui, &hit);
    failed += assert_true(ui.screen == GAME_UI_SCREEN_MENU, "back returns to menu");

    game_default_scene_shutdown(&scene);
    return failed;
}

int test_ui_state(void)
{
    int failed = 0;
    failed += test_ui_layout_stability();
    failed += test_ui_hit_testing_and_selection();

    if (failed == 0) {
        printf("[ui_state] PASS\n");
    }
    return failed;
}
