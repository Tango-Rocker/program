#include <stdio.h>

#include "event/event.h"
#include "event/event_log.h"
#include "sim/sensory_fields.h"
#include "world/field_registry.h"
#include "world/tile_field.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[sensory_fields] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_sensory_fields(void) {
    int failed = 0;

    GameFieldRegistrySlot slots[4] = {0};
    GameFieldRegistry registry = {0};
    failed += assert_true(
        game_field_registry_init(&registry, slots, 4u) == GAME_FIELD_REGISTRY_RESULT_OK,
        "field registry init"
    );

    GameTileFieldConfig field_config = {
        .q_min = 0,
        .r_min = 0,
        .q_count = 5u,
        .r_count = 5u,
        .min_value = 0,
        .max_value = 64,
    };
    failed += assert_true(
        game_field_registry_define_field(&registry, GAME_FIELD_ID_LIGHT, &field_config) == GAME_FIELD_REGISTRY_RESULT_OK,
        "define light field"
    );
    failed += assert_true(
        game_field_registry_define_field(&registry, GAME_FIELD_ID_SCENT, &field_config) == GAME_FIELD_REGISTRY_RESULT_OK,
        "define scent field"
    );
    failed += assert_true(
        game_field_registry_define_field(&registry, GAME_FIELD_ID_BLOOD, &field_config) == GAME_FIELD_REGISTRY_RESULT_OK,
        "define blood field"
    );

    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 4u) == GAME_EVENT_LOG_RESULT_OK, "event log init");

    GameSensoryFieldImpulseResult light_result = {0};
    failed += assert_true(
        game_sensory_apply_light_impulse(
            &registry,
            (GameHexAxial){0, 0},
            12,
            2,
            1,
            1u,
            1u,
            &log,
            &light_result
        ) == GAME_SENSORY_FIELDS_RESULT_OK,
        "apply light impulse"
    );
    failed += assert_true(light_result.updated_tiles > 0u, "light updated tiles");
    failed += assert_true(light_result.event_appended, "light emitted trace");

    GameSensoryFieldImpulseResult scent_result = {0};
    failed += assert_true(
        game_sensory_apply_scent_impulse(
            &registry,
            (GameHexAxial){0, 0},
            9,
            2,
            1,
            2u,
            2u,
            &log,
            &scent_result
        ) == GAME_SENSORY_FIELDS_RESULT_OK,
        "apply scent impulse"
    );
    failed += assert_true(scent_result.updated_tiles > 0u, "scent updated tiles");

    GameSensoryFieldImpulseResult blood_result = {0};
    failed += assert_true(
        game_sensory_apply_blood_impulse(
            &registry,
            (GameHexAxial){0, 0},
            6,
            1,
            1,
            3u,
            3u,
            &log,
            &blood_result
        ) == GAME_SENSORY_FIELDS_RESULT_OK,
        "apply blood impulse"
    );

    int32_t light_center = 0;
    int32_t scent_center = 0;
    int32_t blood_center = 0;
    failed += assert_true(
        game_sensory_sample_field(&registry, GAME_FIELD_ID_LIGHT, (GameHexAxial){0, 0}, &light_center)
            == GAME_SENSORY_FIELDS_RESULT_OK,
        "sample light center"
    );
    failed += assert_true(
        game_sensory_sample_field(&registry, GAME_FIELD_ID_SCENT, (GameHexAxial){0, 0}, &scent_center)
            == GAME_SENSORY_FIELDS_RESULT_OK,
        "sample scent center"
    );
    failed += assert_true(
        game_sensory_sample_field(&registry, GAME_FIELD_ID_BLOOD, (GameHexAxial){0, 0}, &blood_center)
            == GAME_SENSORY_FIELDS_RESULT_OK,
        "sample blood center"
    );
    failed += assert_true(light_center != scent_center || blood_center != scent_center, "independent field behavior");

    int32_t light_affected = 0;
    int32_t scent_affected = 0;
    int32_t blood_affected = 0;
    failed += assert_true(game_sensory_decay_light_field(&registry, 3, &light_affected) == GAME_SENSORY_FIELDS_RESULT_OK, "decay light");
    failed += assert_true(game_sensory_decay_scent_field(&registry, 1, &scent_affected) == GAME_SENSORY_FIELDS_RESULT_OK, "decay scent");
    failed += assert_true(game_sensory_decay_blood_field(&registry, 1, &blood_affected) == GAME_SENSORY_FIELDS_RESULT_OK, "decay blood");

    failed += assert_true(light_affected > 0, "decay touched light tiles");
    failed += assert_true(scent_affected > 0, "decay touched scent tiles");
    failed += assert_true(blood_affected > 0, "decay touched blood tiles");

    failed += assert_true(
        game_sensory_sample_field(&registry, GAME_FIELD_ID_LIGHT, (GameHexAxial){0, 0}, &light_center) == GAME_SENSORY_FIELDS_RESULT_OK,
        "resample light after decay"
    );
    failed += assert_true(light_center < 12, "light center decayed");
    failed += assert_true(
        game_sensory_sample_field(&registry, GAME_FIELD_ID_SCENT, (GameHexAxial){0, 0}, &scent_center) == GAME_SENSORY_FIELDS_RESULT_OK,
        "resample scent after decay"
    );
    failed += assert_true(scent_center < 9, "scent center decayed");
    failed += assert_true(
        game_sensory_sample_field(&registry, GAME_FIELD_ID_BLOOD, (GameHexAxial){0, 0}, &blood_center) == GAME_SENSORY_FIELDS_RESULT_OK,
        "resample blood after decay"
    );
    failed += assert_true(blood_center < 6, "blood center decayed");
    failed += assert_true(game_event_log_count(&log) >= 3u, "trace entries for impulses");

    game_event_log_destroy(&log);
    game_field_registry_destroy(&registry);

    if (failed == 0) {
        printf("[sensory_fields] PASS\n");
    }
    return failed;
}
