#include <stdio.h>
#include <string.h>

#include "event/event.h"
#include "event/event_log.h"
#include "sim/command.h"
#include "sim/noise_system.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[noise] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_noise_field(void) {
    int failed = 0;

    GameTileField field = {0};
    GameTileFieldConfig field_config = {
        .q_min = -2,
        .r_min = -2,
        .q_count = 5u,
        .r_count = 5u,
        .min_value = 0,
        .max_value = 100,
    };
    failed += assert_true(game_tile_field_init(&field, &field_config) == GAME_TILE_FIELD_RESULT_OK, "noise field init");

    GameEventQueue queue = {0};
    GameEventQueueResult qres = game_event_queue_init(&queue, 4u);
    failed += assert_true(qres == GAME_EVENT_QUEUE_RESULT_OK, "event queue init");

    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 4u) == GAME_EVENT_LOG_RESULT_OK, "event log init");

    GameCommand command = {0};
    GameCommandNoisePayload payload = {
        .origin_q = 0,
        .origin_r = 0,
        .intensity = 10,
        .max_radius = 2,
        .attenuation = 1,
        .decay_ticks = 0,
    };
    command.type = GAME_COMMAND_TYPE_EMIT_NOISE;
    command.requested_tick = 7u;
    command.source = (GameEntityId){0u, 0u};
    command.payload_size = sizeof(payload);
    memcpy(command.payload.bytes, &payload, sizeof(payload));

    GameNoiseAppliedResult result = {0};
    failed += assert_true(
        game_noise_emit_from_command(&command, 7u, &queue, &log, &field, &result) == GAME_NOISE_RESULT_OK,
        "emit noise command"
    );
    failed += assert_true(result.noise_event_seq == 1u, "noise event sequence");
    failed += assert_true(result.field_event_appended, "field impulse event appended");
    failed += assert_true(result.updated_tiles > 0u, "tiles updated");
    failed += assert_true(result.updated_tiles == 19u, "radius 2 updates 19");

    const GameEventLogEntry *first = game_event_log_at(&log, 0u);
    const GameEventLogEntry *second = game_event_log_at(&log, 1u);
    failed += assert_true(first && second, "log entries exist");
    if (first && second) {
        failed += assert_true(second->parent_sequence == first->sequence, "field event parent chain");
    }

    int32_t center = 0;
    failed += assert_true(game_tile_field_get(&field, (GameHexAxial){0, 0}, &center) == GAME_TILE_FIELD_RESULT_OK, "sample center after impulse");
    failed += assert_true(center > 0, "center changed");

    game_event_queue_destroy(&queue);
    game_event_log_destroy(&log);
    game_tile_field_destroy(&field);

    if (failed == 0) {
        printf("[noise] PASS\n");
    }
    return failed;
}
