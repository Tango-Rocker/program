#include <stdio.h>

#include "world/topology.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[topology] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static bool topology_passable(int32_t value) {
    return value == 1;
}

static bool topology_portal(int32_t value) {
    return value == 2;
}

static int ensure_tile(GameWorldMap *map, int32_t tile_radius, GameHexAxial tile, int32_t value) {
    GameHexChunkKey chunk_key = game_hex_cube_to_chunk_key(game_hex_axial_to_cube(tile), tile_radius);
    GameWorldMapResult create = game_world_map_create_chunk(map, chunk_key);
    if (create != GAME_WORLD_MAP_RESULT_OK && create != GAME_WORLD_MAP_RESULT_CHUNK_EXISTS) {
        return 1;
    }
    return game_world_map_set(map, tile, value) != GAME_WORLD_MAP_RESULT_OK;
}

int test_topology(void) {
    int failed = 0;
    GameWorldMap map = {0};
    failed += assert_true(game_world_map_init(&map, 4, -1) == GAME_WORLD_MAP_RESULT_OK, "init topology map");

    int failed_a = ensure_tile(&map, 4, (GameHexAxial){0, 0}, 1);
    failed += assert_true(failed_a == 0, "set region A tile");
    failed += assert_true(ensure_tile(&map, 4, (GameHexAxial){1, 0}, 1) == 0, "set region A tile 2");
    failed += assert_true(ensure_tile(&map, 4, (GameHexAxial){0, 1}, 1) == 0, "set region A tile 3");
    failed += assert_true(ensure_tile(&map, 4, (GameHexAxial){3, 0}, 1) == 0, "set region B tile");
    failed += assert_true(ensure_tile(&map, 4, (GameHexAxial){4, 0}, 1) == 0, "set region B tile 2");

    GameWorldTopology topology = {0};
    game_world_topology_init(&topology);

    failed += assert_true(
        game_world_topology_rebuild(&topology, &map, topology_passable, NULL) == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "rebuild topology base map"
    );
    failed += assert_true(game_world_topology_region_count(&topology) == 2u, "region count base disconnected");
    failed += assert_true(game_world_topology_same_region(&topology, (GameHexAxial){0, 0}, (GameHexAxial){1, 0}),
                        "base same-region A");
    failed += assert_true(!game_world_topology_same_region(&topology, (GameHexAxial){0, 0}, (GameHexAxial){3, 0}),
                        "base different regions disconnected");

    GameWorldTopologyRegionId region_a_one = 0u;
    GameWorldTopologyRegionId region_b_one = 0u;
    failed += assert_true(
        game_world_topology_region_of_tile(&topology, (GameHexAxial){0, 0}, &region_a_one) == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "lookup region A"
    );
    failed += assert_true(
        game_world_topology_region_of_tile(&topology, (GameHexAxial){3, 0}, &region_b_one) == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "lookup region B"
    );

    failed += assert_true(ensure_tile(&map, 4, (GameHexAxial){2, 0}, 2) == 0, "set bridge portal");
    failed += assert_true(
        game_world_topology_rebuild(&topology, &map, topology_passable, topology_portal) == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "rebuild topology with portal"
    );
    failed += assert_true(game_world_topology_region_count(&topology) == 2u, "portal does not merge regions");
    failed += assert_true(game_world_topology_portal_count(&topology) == 1u, "single portal connection");

    GameWorldTopologyPortal portal = {0};
    failed += assert_true(game_world_topology_portal_at(&topology, 0u, &portal) == GAME_WORLD_TOPOLOGY_RESULT_OK,
                        "portal accessible");
    failed += assert_true(portal.portal.q == 2 && portal.portal.r == 0, "portal tile is bridge");

    GameWorldTopologyRegionId region_min = region_a_one < region_b_one ? region_a_one : region_b_one;
    GameWorldTopologyRegionId region_max = region_a_one < region_b_one ? region_b_one : region_a_one;
    failed += assert_true((portal.region_a == region_min && portal.region_b == region_max)
                          || (portal.region_a == region_max && portal.region_b == region_min),
                        "portal connects expected regions");
    failed += assert_true(!game_world_topology_same_region(&topology, (GameHexAxial){0, 0}, (GameHexAxial){4, 0}),
                        "portal keeps split regions");

    failed += assert_true(ensure_tile(&map, 4, (GameHexAxial){2, 0}, 0) == 0, "remove portal");
    failed += assert_true(
        game_world_topology_rebuild(&topology, &map, topology_passable, topology_portal) == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "rebuild topology after blocking bridge"
    );
    failed += assert_true(game_world_topology_region_count(&topology) == 2u, "blocking keeps split regions");
    failed += assert_true(game_world_topology_portal_count(&topology) == 0u, "blocking removes portal");

    GameWorldTopologyRegionId region_a_two = 0u;
    GameWorldTopologyRegionId region_b_two = 0u;
    failed += assert_true(
        game_world_topology_region_of_tile(&topology, (GameHexAxial){0, 0}, &region_a_two) == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "rebuild region A lookup"
    );
    failed += assert_true(
        game_world_topology_region_of_tile(&topology, (GameHexAxial){3, 0}, &region_b_two) == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "rebuild region B lookup"
    );
    failed += assert_true(region_a_one == region_a_two && region_b_one == region_b_two,
                        "stable region ids after blocking rebuild");

    failed += assert_true(ensure_tile(&map, 4, (GameHexAxial){2, 0}, 1) == 0, "restore bridge passable");
    failed += assert_true(
        game_world_topology_rebuild(&topology, &map, topology_passable, NULL) == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "rebuild topology with restored bridge"
    );
    failed += assert_true(game_world_topology_region_count(&topology) == 1u, "bridge connects all regions");
    failed += assert_true(game_world_topology_same_region(&topology, (GameHexAxial){0, 0}, (GameHexAxial){4, 0}),
                        "regions now connected");

    game_world_topology_destroy(&topology);
    game_world_map_destroy(&map);

    if (failed == 0) {
        printf("[topology] PASS\n");
    }
    return failed;
}
