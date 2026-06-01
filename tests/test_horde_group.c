#include <stdio.h>

#include "event/event.h"
#include "event/event_log.h"
#include "sim/horde_group.h"
#include "world/topology.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[horde_group] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_horde_group(void) {
    int failed = 0;

    GameWorldTopologyPortal portals[2] = {
        { .region_a = 1u, .region_b = 2u, .portal = {0, 0} },
        { .region_a = 1u, .region_b = 3u, .portal = {1, 0} },
    };
    GameWorldTopology topology = {
        .portals = portals,
        .portal_count = 2u,
    };

    GameWorldTopologyRegionId neighbors[2] = {0};
    size_t neighbor_count = 0u;
    GameHordeGroupConfig config = {
        .pressure_gain_per_sample = 4u,
        .decay_per_tick = 1u,
        .threatened_threshold = 12u,
        .calm_threshold = 3u,
        .migration_threshold = 20u,
        .migration_transfer = 5u,
    };

    failed +=
        assert_true(game_horde_group_collect_neighbors(&topology, 1u, neighbors, 2u, &neighbor_count) == GAME_HORDE_GROUP_RESULT_OK,
                    "collect neighbors");
    failed += assert_true(neighbor_count == 2u, "both neighbors collected");

    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 8u) == GAME_EVENT_LOG_RESULT_OK, "event log init");

    GameHordeGroupState group = {0};
    game_horde_group_init(&group, 42u, 1u);
    GameHordeGroupUpdateResult update = {0};
    uint32_t samples[2] = {30u, 30u};
    failed += assert_true(
        game_horde_group_tick(
            &group,
            &config,
            6u,
            (GameEntityId){10u, 0u},
            neighbors,
            samples,
            neighbor_count,
            1u,
            1u,
            &log,
            &update
        ) == GAME_HORDE_GROUP_RESULT_OK,
        "group tick with migration input"
    );
    failed += assert_true(update.migrated, "group migrated to deterministic lowest region id");
    failed += assert_true(update.migrated_pressure > 0u, "migration pressure moved");
    failed += assert_true(update.current_region == 2u, "tie-break picked lower region id");
    failed += assert_true(update.current_pressure == 19u, "migration reduced gained source pressure");
    failed += assert_true(update.current_posture == GAME_HORDE_POSTURE_THREATENED, "posture threatened after pressure gain");

    GameEventLogEntry *trace = game_event_log_at(&log, 0u);
    failed += assert_true(trace != NULL && trace->type == GAME_EVENT_TYPE_HORDE_GROUP_UPDATED, "group update trace emitted");

    uint32_t region_samples[] = {1u, 0u, 0u};
    GameWorldTopologyRegionId region_ids[] = {1u, 2u, 3u};
    failed += assert_true(
        game_horde_group_tick(
            &group,
            &config,
            0u,
            (GameEntityId){10u, 0u},
            region_ids,
            region_samples,
            0u,
            2u,
            2u,
            &log,
            &update
        ) == GAME_HORDE_GROUP_RESULT_OK,
        "zero neighbor count ignores arrays"
    );
    failed += assert_true(
        game_horde_group_tick(
            &group,
            &config,
            0u,
            (GameEntityId){10u, 0u},
            NULL,
            region_samples,
            2u,
            3u,
            3u,
            &log,
            &update
        ) == GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT,
        "null neighbors rejected when sample count present"
    );

    game_event_log_destroy(&log);

    if (failed == 0) {
        printf("[horde_group] PASS\n");
    }
    return failed;
}
