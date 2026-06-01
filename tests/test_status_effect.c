#include <stdio.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "event/event_queue.h"
#include "sim/status_effect.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[status_effect] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_status_effect(void) {
    int failed = 0;
    GameEntityRegistry registry = {0};
    failed += assert_true(game_entity_registry_init(&registry, 4u) == GAME_ENTITY_RESULT_OK, "entity registry init");
    GameEntityId target = {0u, 0u};
    failed += assert_true(game_entity_registry_create(&registry, &target) == GAME_ENTITY_RESULT_OK, "target entity created");

    GameStatusEffectInstance slots[4] = {0};
    GameStatusEffectTable table = {0};
    failed += assert_true(game_status_effect_init(&table, slots, 4u) == GAME_STATUS_EFFECT_RESULT_OK, "status effect init");

    GameEventQueue queue = {0};
    failed += assert_true(game_event_queue_init(&queue, 4u) == GAME_EVENT_QUEUE_RESULT_OK, "status event queue init");
    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 8u) == GAME_EVENT_LOG_RESULT_OK, "status event log init");

    GameStatusEffectHandle handle = {0u, 0u};
    failed += assert_true(
        game_status_effect_apply(
            &table,
            &registry,
            target,
            target,
            1u,
            2,
            3u,
            7,
            8,
            0u,
            &queue,
            &log,
            100u,
            &handle
        ) == GAME_STATUS_EFFECT_RESULT_OK,
        "status apply first"
    );
    failed += assert_true(game_status_effect_count(&table) == 1u, "status active count");

    GameStatusEffectHandle duplicate = handle;
    failed += assert_true(
        game_status_effect_apply(
            &table,
            &registry,
            target,
            target,
            1u,
            5,
            5u,
            7,
            8,
            1u,
            &queue,
            &log,
            100u,
            &duplicate
        ) == GAME_STATUS_EFFECT_RESULT_OK,
        "status duplicate apply refreshes"
    );
    failed += assert_true(game_status_effect_count(&table) == 1u, "status no stacking duplicate");
    failed += assert_true(handle.slot == duplicate.slot && handle.generation == duplicate.generation, "status handle stable on refresh");

    GameEvent event = {0};
    failed += assert_true(game_event_queue_pop(&queue, &event) == GAME_EVENT_QUEUE_RESULT_OK, "status first apply queue entry");
    failed += assert_true(event.type == GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED, "status applied event type");
    failed += assert_true(game_event_queue_pop(&queue, &event) == GAME_EVENT_QUEUE_RESULT_OK, "status refresh queue entry");
    failed += assert_true(event.type == GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED, "status refresh event type");
    const GameStatusEffectPayload *applied_payload = (const GameStatusEffectPayload *)event.payload.bytes;
    failed += assert_true(applied_payload->magnitude == 5, "status payload magnitude after refresh");

    failed += assert_true(
        game_status_effect_advance(&table, 5u, &queue, &log) == GAME_STATUS_EFFECT_RESULT_OK,
        "status advance without expiry"
    );
    failed += assert_true(game_status_effect_count(&table) == 1u, "status still active before expiry");

    failed += assert_true(
        game_status_effect_advance(&table, 7u, &queue, &log) == GAME_STATUS_EFFECT_RESULT_OK,
        "status advance expiry tick"
    );
    failed += assert_true(game_status_effect_count(&table) == 0u, "status expired");
    failed += assert_true(game_event_queue_pop(&queue, &event) == GAME_EVENT_QUEUE_RESULT_OK, "status expire queue entry");
    failed += assert_true(event.type == GAME_EVENT_TYPE_STATUS_EFFECT_EXPIRED, "status expired event type");
    failed += assert_true(game_event_log_count(&log) == 3u, "status logs applied, refreshed, and expired");

    failed += assert_true(game_entity_registry_destroy_id(&registry, target) == GAME_ENTITY_RESULT_OK, "destroy target");
    failed += assert_true(
        game_status_effect_apply(
            &table,
            &registry,
            target,
            target,
            2u,
            3,
            1u,
            1,
            2,
            8u,
            NULL,
            &log,
            100u,
            &handle
        ) == GAME_STATUS_EFFECT_RESULT_INVALID_HANDLE,
        "stale target rejected"
    );

    game_event_queue_destroy(&queue);
    game_event_log_destroy(&log);
    game_entity_registry_destroy(&registry);

    if (failed == 0) {
        printf("[status_effect] PASS\n");
    }
    return failed;
}
