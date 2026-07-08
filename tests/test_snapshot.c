#include <stdio.h>
#include <string.h>

#include "ecs/entity.h"
#include "sim/scheduler.h"
#include "sim/snapshot.h"
#include "sim/sim_context.h"
#include "world/world_map.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[snapshot] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int assert_equal_u32(uint32_t lhs, uint32_t rhs, const char *label) {
    if (lhs != rhs) {
        printf("[snapshot] FAIL: %s (%u != %u)\n", label, lhs, rhs);
        return 1;
    }
    return 0;
}

static int test_snapshot_round_trip(void) {
    int failed = 0;

    GameSimContext context = {0};
    GameSimContextConfig context_cfg = {
        .rng_seed = 12345u,
        .initial_entity_capacity = 4u,
        .initial_event_capacity = 8u,
    };
    failed += assert_true(game_sim_context_init(&context, &context_cfg) == GAME_SIM_CONTEXT_RESULT_OK, "init context");

    GameEntityId first = {UINT32_MAX, UINT32_MAX};
    GameEntityId second = {UINT32_MAX, UINT32_MAX};
    failed += assert_true(game_entity_registry_create(&context.entity_registry, &first) == GAME_ENTITY_RESULT_OK, "create first entity");
    failed += assert_true(game_entity_registry_create(&context.entity_registry, &second) == GAME_ENTITY_RESULT_OK, "create second entity");
    failed += assert_true(game_entity_registry_destroy_id(&context.entity_registry, second) == GAME_ENTITY_RESULT_OK, "destroy second entity");

    context.tick = 77u;

    GameScheduler scheduler = {0};
    failed += assert_true(game_scheduler_init(&scheduler, 3u) == GAME_SCHEDULER_RESULT_OK, "init scheduler");

    uint8_t payload[1] = {42u};
    GameSchedulerHandle scheduled = {0};
    failed += assert_true(
        game_scheduler_schedule(
            &scheduler,
            12u,
            3u,
            first,
            payload,
            sizeof(payload),
            &scheduled
        ) == GAME_SCHEDULER_RESULT_OK,
        "schedule for snapshot"
    );

    GameWorldMap map = {0};
    failed += assert_true(game_world_map_init(&map, 2, -1) == GAME_WORLD_MAP_RESULT_OK, "init map");
    GameHexChunkKey key_a = game_hex_cube_to_chunk_key(game_hex_axial_to_cube((GameHexAxial){0, 0}), 2);
    GameHexChunkKey key_b = game_hex_cube_to_chunk_key(game_hex_axial_to_cube((GameHexAxial){3, 0}), 2);
    failed += assert_true(game_world_map_create_chunk(&map, key_a) == GAME_WORLD_MAP_RESULT_OK, "create chunk a");
    failed += assert_true(game_world_map_create_chunk(&map, key_b) == GAME_WORLD_MAP_RESULT_OK, "create chunk b");
    failed += assert_true(game_world_map_set(&map, (GameHexAxial){0, 0}, 4) == GAME_WORLD_MAP_RESULT_OK, "set value a");
    failed += assert_true(game_world_map_set(&map, (GameHexAxial){3, 0}, 9) == GAME_WORLD_MAP_RESULT_OK, "set value b");

    size_t required = game_snapshot_required_size(&context, &map, &scheduler);
    failed += assert_true(required > 0u, "snapshot size computed");

    unsigned char buffer[4096] = {0};
    size_t used = 0u;
    failed +=
        assert_true(game_snapshot_serialize(&context, &map, &scheduler, buffer, sizeof(buffer), &used) == GAME_SNAPSHOT_RESULT_OK, "serialize");
    failed += assert_true(used == required, "serialize length exact");

    GameSimContext restored_context = {0};
    GameWorldMap restored_map = {0};
    GameScheduler restored_scheduler = {0};
    failed += assert_true(
        game_snapshot_deserialize(buffer, used, NULL, &restored_context, &restored_map, &restored_scheduler)
            == GAME_SNAPSHOT_RESULT_OK,
        "deserialize"
    );

    failed += assert_equal_u32((uint32_t)restored_context.entity_registry.capacity, 4u, "restored entity capacity");
    failed += assert_true(restored_context.tick == 77u, "restored tick");
    failed += assert_equal_u32(restored_context.entity_registry.free_count, 1u, "restored free count");
    failed += assert_true(
        restored_context.entity_registry.generations[first.index] == context.entity_registry.generations[first.index],
        "first generation restored"
    );
    failed += assert_true(
        restored_context.entity_registry.alive[first.index] == context.entity_registry.alive[first.index],
        "first alive restored"
    );
    failed += assert_equal_u32((uint32_t)restored_map.chunk_count, 2u, "restored chunk count");
    failed += assert_true(
        game_world_map_get(&restored_map, (GameHexAxial){0, 0}, &(int32_t){0}) == GAME_WORLD_MAP_RESULT_OK,
        "restored map tile query"
    );

    int32_t restored_value = 0;
    failed += assert_true(
        game_world_map_get(&restored_map, (GameHexAxial){0, 0}, &restored_value) == GAME_WORLD_MAP_RESULT_OK,
        "restore value query");
    failed += assert_true(restored_value == 4, "restore chunk value a");

    failed += assert_true(restored_scheduler.active_count == 1u, "restored scheduler count");
    failed += assert_true(restored_scheduler.next_sequence == scheduler.next_sequence, "restored scheduler sequence");

    failed +=
        assert_true(
            game_world_map_get(&restored_map, (GameHexAxial){3, 0}, &restored_value) == GAME_WORLD_MAP_RESULT_OK
            && restored_value == 9,
            "restore chunk value b"
        );

    GameSchedulerEntry *found = NULL;
    for (size_t i = 0; i < restored_scheduler.capacity; ++i) {
        if (restored_scheduler.entries[i].occupied && restored_scheduler.entries[i].due_tick == 12u) {
            found = &restored_scheduler.entries[i];
            break;
        }
    }
    failed += assert_true(found != NULL, "deserialized event exists");
    failed += assert_true(found == NULL || found->payload_size == 1u, "restored payload size");
    failed += assert_true(found == NULL || found->payload[0] == 42u, "restored payload content");

    game_sim_context_shutdown(&context);
    game_sim_context_shutdown(&restored_context);
    game_scheduler_destroy(&scheduler);
    game_world_map_destroy(&map);
    game_scheduler_destroy(&restored_scheduler);
    game_world_map_destroy(&restored_map);

    if (failed == 0) {
        printf("[snapshot] PASS\n");
    }

    return failed;
}

int test_snapshot(void) {
    int failed = 0;
    failed += test_snapshot_round_trip();
    return failed;
}
