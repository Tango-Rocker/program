#include <stdio.h>

#include "world/field_registry.h"
#include "world/tile_field.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[field_registry] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_field_registry(void) {
    int failed = 0;

    GameFieldRegistrySlot slots[3] = {0};
    GameFieldRegistry registry = {0};
    failed += assert_true(
        game_field_registry_init(&registry, slots, 3u) == GAME_FIELD_REGISTRY_RESULT_OK,
        "field registry init"
    );

    GameTileFieldConfig field_config = {
        .q_min = 0,
        .r_min = 0,
        .q_count = 2u,
        .r_count = 2u,
        .min_value = 0,
        .max_value = 100,
    };
    failed += assert_true(
        game_field_registry_define_field(&registry, GAME_FIELD_ID_NOISE, &field_config) == GAME_FIELD_REGISTRY_RESULT_OK,
        "define noise field"
    );
    failed += assert_true(
        game_field_registry_define_field(&registry, GAME_FIELD_ID_LIGHT, &field_config) == GAME_FIELD_REGISTRY_RESULT_OK,
        "define light field"
    );
    failed += assert_true(
        game_field_registry_define_field(&registry, GAME_FIELD_ID_SCENT, &field_config) == GAME_FIELD_REGISTRY_RESULT_OK,
        "define scent field"
    );

    GameTileField *noise_field = NULL;
    failed += assert_true(
        game_field_registry_get(&registry, GAME_FIELD_ID_NOISE, &noise_field) == GAME_FIELD_REGISTRY_RESULT_OK,
        "get noise field"
    );
    GameTileField *light_field = NULL;
    failed += assert_true(
        game_field_registry_get(&registry, GAME_FIELD_ID_LIGHT, &light_field) == GAME_FIELD_REGISTRY_RESULT_OK,
        "get light field"
    );
    GameTileField *scent_field = NULL;
    failed += assert_true(
        game_field_registry_get(&registry, GAME_FIELD_ID_SCENT, &scent_field) == GAME_FIELD_REGISTRY_RESULT_OK,
        "get scent field"
    );

    failed += assert_true(game_tile_field_set(noise_field, (GameHexAxial){0, 0}, 10) == GAME_TILE_FIELD_RESULT_OK, "set noise value");
    failed += assert_true(game_tile_field_set(light_field, (GameHexAxial){0, 0}, 20) == GAME_TILE_FIELD_RESULT_OK, "set light value");
    failed += assert_true(game_tile_field_set(scent_field, (GameHexAxial){0, 0}, 30) == GAME_TILE_FIELD_RESULT_OK, "set scent value");

    int32_t noise_value = 0;
    int32_t light_value = 0;
    int32_t scent_value = 0;
    failed += assert_true(game_tile_field_get(noise_field, (GameHexAxial){0, 0}, &noise_value) == GAME_TILE_FIELD_RESULT_OK, "get noise");
    failed += assert_true(game_tile_field_get(light_field, (GameHexAxial){0, 0}, &light_value) == GAME_TILE_FIELD_RESULT_OK, "get light");
    failed += assert_true(game_tile_field_get(scent_field, (GameHexAxial){0, 0}, &scent_value) == GAME_TILE_FIELD_RESULT_OK, "get scent");
    failed += assert_true(noise_value == 10, "noise value independent");
    failed += assert_true(light_value == 20, "light value independent");
    failed += assert_true(scent_value == 30, "scent value independent");

    game_tile_field_add(noise_field, (GameHexAxial){1, 1}, 5);
    game_tile_field_add(light_field, (GameHexAxial){1, 1}, 6);
    game_tile_field_add(scent_field, (GameHexAxial){1, 1}, 8);
    game_tile_field_decay(noise_field, 2);
    game_tile_field_decay(light_field, 3);
    game_tile_field_decay(scent_field, 4);

    failed += assert_true(game_tile_field_get(noise_field, (GameHexAxial){1, 1}, &noise_value) == GAME_TILE_FIELD_RESULT_OK, "decayed noise sample");
    failed += assert_true(game_tile_field_get(light_field, (GameHexAxial){1, 1}, &light_value) == GAME_TILE_FIELD_RESULT_OK, "decayed light sample");
    failed += assert_true(game_tile_field_get(scent_field, (GameHexAxial){1, 1}, &scent_value) == GAME_TILE_FIELD_RESULT_OK, "decayed scent sample");
    failed += assert_true(noise_value != scent_value, "independent decay changed field values");

    failed += assert_true(game_field_registry_enable(&registry, GAME_FIELD_ID_LIGHT, false) == GAME_FIELD_REGISTRY_RESULT_OK, "disable light");
    GameTileField *disabled = NULL;
    failed += assert_true(
        game_field_registry_get(&registry, GAME_FIELD_ID_LIGHT, &disabled) == GAME_FIELD_REGISTRY_RESULT_DISABLED,
        "disabled field blocked"
    );

    GameFieldId disabled_ids[2] = {0};
    size_t disabled_count = 0u;
    failed += assert_true(
        game_field_registry_collect_ids(&registry, disabled_ids, 2u, &disabled_count) == GAME_FIELD_REGISTRY_RESULT_OK,
        "collect skips disabled fields"
    );
    failed += assert_true(disabled_count == 2u, "collect ignores disabled light");
    failed += assert_true(disabled_ids[0] == GAME_FIELD_ID_NOISE, "collect still starts at noise");
    failed += assert_true(disabled_ids[1] == GAME_FIELD_ID_SCENT, "collect still deterministic with light disabled");

    failed += assert_true(game_field_registry_enable(&registry, GAME_FIELD_ID_LIGHT, true) == GAME_FIELD_REGISTRY_RESULT_OK, "re-enable light");

    GameFieldId ids[3] = {0};
    size_t collected = 0u;
    failed += assert_true(
        game_field_registry_collect_ids(&registry, ids, 3u, &collected) == GAME_FIELD_REGISTRY_RESULT_OK,
        "collect ids"
    );
    failed += assert_true(collected == 3u, "collected three ids");
    failed += assert_true(ids[0] == GAME_FIELD_ID_NOISE, "stable deterministic id order[0]");
    failed += assert_true(ids[1] == GAME_FIELD_ID_LIGHT, "stable deterministic id order[1]");
    failed += assert_true(ids[2] == GAME_FIELD_ID_SCENT, "stable deterministic id order[2]");

    game_field_registry_destroy(&registry);

    if (failed == 0) {
        printf("[field_registry] PASS\n");
    }
    return failed;
}
