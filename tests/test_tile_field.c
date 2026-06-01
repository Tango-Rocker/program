#include <stdio.h>

#include "world/tile_field.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[tile_field] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_tile_field(void) {
    int failed = 0;
    GameTileField field = {0};

    GameTileFieldConfig config = {
        .q_min = -1,
        .r_min = -1,
        .q_count = 3u,
        .r_count = 3u,
        .min_value = -5,
        .max_value = 10,
    };

    failed += assert_true(game_tile_field_init(&field, &config) == GAME_TILE_FIELD_RESULT_OK, "init field");

    failed += assert_true(game_tile_field_set(&field, (GameHexAxial){0, 0}, 5) == GAME_TILE_FIELD_RESULT_OK, "set in bounds");
    int32_t value = 0;
    failed += assert_true(game_tile_field_get(&field, (GameHexAxial){0, 0}, &value) == GAME_TILE_FIELD_RESULT_OK, "get in bounds");
    failed += assert_true(value == 5, "value equals set");

    failed += assert_true(game_tile_field_add(&field, (GameHexAxial){0, 0}, 20) == GAME_TILE_FIELD_RESULT_OK, "add with clamping");
    failed += assert_true(game_tile_field_get(&field, (GameHexAxial){0, 0}, &value) == GAME_TILE_FIELD_RESULT_OK, "get after add");
    failed += assert_true(value == 10, "value clamped to max");

    failed += assert_true(game_tile_field_get(&field, (GameHexAxial){5, 5}, &value) == GAME_TILE_FIELD_RESULT_OUT_OF_BOUNDS, "oob get");
    failed += assert_true(game_tile_field_set(&field, (GameHexAxial){5, 5}, 1) == GAME_TILE_FIELD_RESULT_OUT_OF_BOUNDS, "oob set");

    failed += assert_true(game_tile_field_decay(&field, 3) == GAME_TILE_FIELD_RESULT_OK, "decay positive");
    failed += assert_true(game_tile_field_get(&field, (GameHexAxial){0, 0}, &value) == GAME_TILE_FIELD_RESULT_OK, "get after decay");
    failed += assert_true(value == 7, "decayed by 3");

    failed += assert_true(game_tile_field_clear(&field, 0) == GAME_TILE_FIELD_RESULT_OK, "clear to zero");
    failed += assert_true(game_tile_field_get(&field, (GameHexAxial){0, 0}, &value) == GAME_TILE_FIELD_RESULT_OK, "get after clear");
    failed += assert_true(value == 0, "clear zero");

    game_tile_field_destroy(&field);
    if (failed == 0) {
        printf("[tile_field] PASS\n");
    }
    return failed;
}
