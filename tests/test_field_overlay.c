#include <stdio.h>

#include "ui/field_overlay.h"
#include "world/field_registry.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[field_overlay] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_field_overlay(void) {
    int failed = 0;
    GameFieldRegistrySlot slots[2] = {0};
    GameFieldRegistry registry = {0};
    GameTileFieldConfig config = {
        .q_min = -1,
        .r_min = -1,
        .q_count = 3u,
        .r_count = 3u,
        .min_value = 0,
        .max_value = 10,
    };

    failed += assert_true(
        game_field_registry_init(&registry, slots, 2u) == GAME_FIELD_REGISTRY_RESULT_OK,
        "registry init"
    );
    failed += assert_true(
        game_field_registry_define_field(&registry, GAME_FIELD_ID_LIGHT, &config) == GAME_FIELD_REGISTRY_RESULT_OK,
        "define field"
    );
    GameTileField *field = NULL;
    failed += assert_true(
        game_field_registry_get(&registry, GAME_FIELD_ID_LIGHT, &field) == GAME_FIELD_REGISTRY_RESULT_OK,
        "get field"
    );
    failed += assert_true(game_tile_field_set(field, (GameHexAxial){0, 0}, 5) == GAME_TILE_FIELD_RESULT_OK, "set value");
    failed += assert_true(game_tile_field_set(field, (GameHexAxial){1, 0}, 10) == GAME_TILE_FIELD_RESULT_OK, "set max");

    GameFieldOverlayCell cells[8] = {0};
    size_t count = 0u;
    GameFieldOverlayBounds bounds = {.min = {0, 0}, .max = {1, 1}};
    failed += assert_true(
        game_field_overlay_extract(&registry, GAME_FIELD_ID_LIGHT, bounds, cells, 8u, &count)
            == GAME_FIELD_OVERLAY_RESULT_OK,
        "extract overlay"
    );
    failed += assert_true(count == 4u, "bounds clipped and ordered");
    failed += assert_true(cells[0].tile.q == 0 && cells[0].tile.r == 0 && cells[0].bucket == 127u, "first cell");
    failed += assert_true(cells[1].tile.q == 1 && cells[1].bucket == 255u, "second cell");

    int32_t unchanged = 0;
    failed += assert_true(game_tile_field_get(field, (GameHexAxial){0, 0}, &unchanged) == GAME_TILE_FIELD_RESULT_OK,
                          "read after extract");
    failed += assert_true(unchanged == 5, "overlay does not mutate field");
    failed += assert_true(
        game_field_overlay_extract(&registry, GAME_FIELD_ID_BLOOD, bounds, cells, 8u, &count)
            == GAME_FIELD_OVERLAY_RESULT_FIELD_NOT_FOUND,
        "missing field rejected"
    );
    failed += assert_true(
        game_field_overlay_extract(&registry, GAME_FIELD_ID_LIGHT, bounds, cells, 1u, &count)
            == GAME_FIELD_OVERLAY_RESULT_BUFFER_TOO_SMALL,
        "small buffer rejected"
    );

    game_field_registry_destroy(&registry);
    if (failed == 0) {
        printf("[field_overlay] PASS\n");
    }
    return failed;
}
