#include <stdbool.h>
#include <stdio.h>

#include "event/event.h"
#include "event/event_log.h"
#include "event/event_queue.h"
#include "sim/projectile.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[projectile] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_projectile(void) {
    int failed = 0;

    GameEventQueue queue = {0};
    failed += assert_true(game_event_queue_init(&queue, 8u) == GAME_EVENT_QUEUE_RESULT_OK, "event queue init");
    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 8u) == GAME_EVENT_LOG_RESULT_OK, "event log init");

    GameProjectileState slots[2] = {0};
    GameProjectileTable table = {0};
    failed += assert_true(game_projectile_init(&table, slots, 2u) == GAME_PROJECTILE_RESULT_OK, "projectile init");

    GameProjectileSpawnRequest impact_request = {
        .source = {4u, 5u},
        .target = {5u, 6u},
        .source_q = 0,
        .source_r = 0,
        .target_q = 2,
        .target_r = 0,
        .damage = 4,
        .speed_per_tick = 1u,
        .remaining_lifetime_ticks = 5,
        .payload_id = 1u,
        .ability_id = 10u,
        .parent_event_sequence = 9u,
    };

    GameProjectileHandle handle = {0u, 0u};
    failed += assert_true(
        game_projectile_spawn(&table, &impact_request, 1u, &handle) == GAME_PROJECTILE_RESULT_OK,
        "projectile spawn"
    );
    failed += assert_true(game_projectile_count(&table) == 1u, "projectile count after spawn");

    failed += assert_true(game_projectile_advance_all(&table, 1u, &queue, &log) == GAME_PROJECTILE_RESULT_OK, "projectile first tick");
    GameProjectileState state = {0};
    failed += assert_true(game_projectile_get(&table, handle, &state) == GAME_PROJECTILE_RESULT_OK, "projectile is active after first tick");
    failed += assert_true(state.current_q == 1 && state.current_r == 0, "projectile moved one tile");

    failed += assert_true(game_projectile_advance_all(&table, 2u, &queue, &log) == GAME_PROJECTILE_RESULT_OK, "projectile second tick");
    failed += assert_true(game_projectile_count(&table) == 0u, "projectile impacts and is removed");

    GameEvent event = {0};
    failed += assert_true(game_event_queue_pop(&queue, &event) == GAME_EVENT_QUEUE_RESULT_OK, "impact event queued");
    failed += assert_true(event.type == GAME_EVENT_TYPE_PROJECTILE_IMPACT, "impact event type");
    failed += assert_true(event.payload_size == sizeof(GameProjectileImpactPayload), "impact payload size");
    const GameProjectileImpactPayload *impact = (const GameProjectileImpactPayload *)event.payload.bytes;
    failed += assert_true(impact->ability_id == 10u, "impact contains ability id");

    failed += assert_true(game_event_queue_pop(&queue, &event) == GAME_EVENT_QUEUE_RESULT_OK, "impact noise event queued");
    failed += assert_true(event.type == GAME_EVENT_TYPE_NOISE_EMITTED, "noise event type");
    failed += assert_true(game_event_log_count(&log) == 2u, "impact chain adds two trace events");
    const GameEventLogEntry *impact_trace = game_event_log_at(&log, 0u);
    const GameEventLogEntry *noise_trace = game_event_log_at(&log, 1u);
    failed += assert_true(impact_trace != NULL && impact_trace->type == GAME_EVENT_TYPE_PROJECTILE_IMPACT, "trace impact type first");
    failed += assert_true(noise_trace != NULL && noise_trace->type == GAME_EVENT_TYPE_NOISE_EMITTED, "trace noise type second");
    failed += assert_true(impact_trace->parent_sequence == 9u, "impact traces to parent ability event");

    GameProjectileState expire_slots[2] = {0};
    GameProjectileTable expire_table = {0};
    failed += assert_true(game_projectile_init(&expire_table, expire_slots, 2u) == GAME_PROJECTILE_RESULT_OK, "expire table init");
    GameProjectileSpawnRequest expire_request = {
        .source = {1u, 2u},
        .target = {3u, 4u},
        .source_q = 0,
        .source_r = 0,
        .target_q = 5,
        .target_r = 0,
        .damage = 1,
        .speed_per_tick = 1u,
        .remaining_lifetime_ticks = 1,
        .payload_id = 2u,
        .ability_id = 11u,
        .parent_event_sequence = 15u,
    };
    failed += assert_true(
        game_projectile_spawn(&expire_table, &expire_request, 1u, &handle) == GAME_PROJECTILE_RESULT_OK,
        "expire projectile spawn"
    );
    failed += assert_true(game_projectile_advance_all(&expire_table, 3u, &queue, &log) == GAME_PROJECTILE_RESULT_OK, "expire tick advances");
    failed += assert_true(game_projectile_count(&expire_table) == 0u, "projectile expired without impact");

    game_event_queue_destroy(&queue);
    game_event_log_destroy(&log);

    if (failed == 0) {
        printf("[projectile] PASS\n");
    }
    return failed;
}
