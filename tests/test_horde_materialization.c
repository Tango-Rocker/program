#include <stdio.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "event/event_queue.h"
#include "sim/horde_group.h"
#include "sim/horde_materialization.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[horde_materialization] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_horde_materialization(void) {
    int failed = 0;

    GameEntityRegistry entity_registry = {0};
    failed += assert_true(game_entity_registry_init(&entity_registry, 8u) == GAME_ENTITY_RESULT_OK, "entity registry init");

    GameHordeGroupState group = {
        .id = 11u,
        .pressure = 20u,
        .mass_estimate = 0u,
        .region_id = 1u,
        .interest_source = {1u, 0u},
        .posture = GAME_HORDE_POSTURE_ALERT,
    };

    GameHordeMaterializationConfig config = {
        .materialize_pressure_threshold = 12u,
        .dematerialize_pressure_threshold = 4u,
        .pressure_cost_per_actor = 4u,
        .pressure_return_per_actor = 2u,
        .spawn_budget_per_tick = 2u,
        .dematerialize_budget_per_tick = 2u,
        .mass_per_actor = 1u,
        .max_spawn_proximity = 5u,
        .spawn_actor_count = 2u,
    };

    GameEntityId actor_ids[4] = {0};
    GameHordeMaterializationState materialization = {0};
    game_horde_materialization_init(&materialization, group.id, actor_ids, 4u);

    GameEventQueue queue = {0};
    failed += assert_true(game_event_queue_init(&queue, 2u) == GAME_EVENT_QUEUE_RESULT_OK, "event queue init");
    GameEventQueue empty_queue = {0};
    failed += assert_true(game_event_queue_init(&empty_queue, 0u) != GAME_EVENT_QUEUE_RESULT_OK, "empty queue rejected");

    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 2u) == GAME_EVENT_LOG_RESULT_OK, "event log init");

    GameHordeMaterializationUpdateResult update = {0};
    failed += assert_true(
        game_horde_materialization_apply(
            &materialization,
            &group,
            &config,
            true,
            1u,
            1u,
            100u,
            &entity_registry,
            &queue,
            &log,
            &update
        ) == GAME_HORDE_MATERIALIZATION_RESULT_OK,
        "materialize from abstract pressure"
    );
    failed += assert_true(update.spawned == true, "spawn action executed");
    failed += assert_true(update.spawned_count == 2u, "spawn budget deterministically used");
    failed += assert_true(materialization.actor_count == 2u, "actor roster recorded");
    failed += assert_true(materialization.actor_ids[0].index == 0u, "first spawned actor id deterministic");
    failed += assert_true(materialization.actor_ids[1].index == 1u, "second spawned actor id deterministic");
    failed += assert_true(group.mass_estimate == 2u, "mass increased");
    failed += assert_true(group.pressure == 12u, "pressure consumed");

    failed += assert_true(game_event_queue_count(&queue) > 0u, "spawn event queued");
    failed += assert_true(game_event_log_count(&log) > 0u, "spawn trace appended");

    GameHordeMaterializationUpdateResult dematerialized = {0};
    group.pressure = 2u;
    failed += assert_true(
        game_horde_materialization_apply(
            &materialization,
            &group,
            &config,
            false,
            0u,
            2u,
            101u,
            &entity_registry,
            &queue,
            &log,
            &dematerialized
        ) == GAME_HORDE_MATERIALIZATION_RESULT_OK,
        "dematerialize when pressure drops"
    );
    failed += assert_true(dematerialized.dematerialized == true, "dematerialize action executed");
    failed += assert_true(dematerialized.dematerialized_count == 2u, "dematerialization budget used");
    failed += assert_true(materialization.actor_count == 0u, "actor roster cleared");
    failed += assert_true(group.pressure > 2u, "pressure returned from actors");
    failed += assert_true(group.mass_estimate == 0u, "mass returned to abstract estimate");

    GameHordeMaterializationUpdateResult skipped = {0};
    group.pressure = 20u;
    group.posture = GAME_HORDE_POSTURE_ALERT;
    failed += assert_true(
        game_horde_materialization_apply(
            &materialization,
            &group,
            &config,
            true,
            100u,
            3u,
            102u,
            &entity_registry,
            &queue,
            &log,
            &skipped
        ) == GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE,
        "out of range proximity blocks spawn"
    );
    failed += assert_true(skipped.spawned == false, "distance gate blocks spawn attempts");
    failed += assert_true(skipped.spawned_count == 0u, "no actors spawned");

    game_event_queue_destroy(&queue);
    game_entity_registry_destroy(&entity_registry);
    game_event_log_destroy(&log);

    if (failed == 0) {
        printf("[horde_materialization] PASS\n");
    }
    return failed;
}
