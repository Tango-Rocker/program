#include <stdio.h>

#include "world/world_map.h"

typedef struct {
    GameHexChunkKey previous_chunk;
    size_t previous_local_index;
    size_t callback_count;
    size_t mismatch_count;
    size_t chunk_switches;
    bool initialized;
    bool chunk_order_ok;
} WorldMapVisitState;

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[world_map] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int world_map_cmp_chunk_key(GameHexChunkKey lhs, GameHexChunkKey rhs) {
    if (lhs.cx < rhs.cx) {
        return -1;
    }
    if (lhs.cx > rhs.cx) {
        return 1;
    }
    if (lhs.cy < rhs.cy) {
        return -1;
    }
    if (lhs.cy > rhs.cy) {
        return 1;
    }
    if (lhs.cz < rhs.cz) {
        return -1;
    }
    if (lhs.cz > rhs.cz) {
        return 1;
    }
    return 0;
}

static size_t world_map_expected_chunk_tile_count(const GameWorldMap *map, GameHexChunkKey chunk_key) {
    size_t count = 0u;
    if (!map || map->chunk_radius <= 0) {
        return 0u;
    }

    int32_t radius = map->chunk_radius;
    int32_t base_y = chunk_key.cy * radius;
    int32_t base_x = chunk_key.cx * radius;
    int32_t base_z = chunk_key.cz * radius;

    for (int32_t lx = 0; lx < radius; ++lx) {
        for (int32_t lz = 0; lz < radius; ++lz) {
            int32_t cube_x = base_x + lx;
            int32_t cube_z = base_z + lz;
            int32_t cube_y = -(cube_x + cube_z);
            if (cube_y >= base_y && cube_y < base_y + radius) {
                ++count;
            }
        }
    }

    return count;
}

static void world_map_collect_visits(void *user, GameWorldMapTileVisit visit) {
    WorldMapVisitState *state = (WorldMapVisitState *)user;

    if (!state->initialized) {
        state->initialized = true;
        state->chunk_order_ok = true;
        state->previous_chunk = visit.chunk_key;
        state->previous_local_index = visit.local_index;
        state->callback_count = 1u;
        return;
    }

    if (world_map_cmp_chunk_key(visit.chunk_key, state->previous_chunk) == 0) {
        if (visit.local_index < state->previous_local_index) {
            ++state->mismatch_count;
        }
        state->previous_local_index = visit.local_index;
    } else {
        if (world_map_cmp_chunk_key(visit.chunk_key, state->previous_chunk) > 0) {
            state->chunk_switches++;
            state->previous_chunk = visit.chunk_key;
            state->previous_local_index = visit.local_index;
        } else {
            state->chunk_order_ok = false;
        }
    }

    ++state->callback_count;
}

int test_world_map(void) {
    int failed = 0;
    GameWorldMap map = {0};
    failed += assert_true(game_world_map_init(&map, 4, -1) == GAME_WORLD_MAP_RESULT_OK, "init world map");

    GameHexAxial left = {3, 0};
    GameHexAxial right = {4, 0};
    GameHexAxial neg = {-1, -2};

    GameHexChunkKey left_key = game_hex_cube_to_chunk_key(game_hex_axial_to_cube(left), 4);
    GameHexChunkKey right_key = game_hex_cube_to_chunk_key(game_hex_axial_to_cube(right), 4);
    GameHexChunkKey neg_key = game_hex_cube_to_chunk_key(game_hex_axial_to_cube(neg), 4);

    failed += assert_true(game_world_map_set(&map, left, 11) == GAME_WORLD_MAP_RESULT_CHUNK_MISSING, "set missing left chunk");
    failed += assert_true(game_world_map_set(&map, right, 22) == GAME_WORLD_MAP_RESULT_CHUNK_MISSING, "set missing right chunk");
    failed += assert_true(game_world_map_set(&map, neg, 33) == GAME_WORLD_MAP_RESULT_CHUNK_MISSING, "set missing negative chunk");

    failed += assert_true(game_world_map_create_chunk(&map, right_key) == GAME_WORLD_MAP_RESULT_OK, "create right chunk");
    failed += assert_true(game_world_map_create_chunk(&map, left_key) == GAME_WORLD_MAP_RESULT_OK, "create left chunk");
    failed += assert_true(game_world_map_create_chunk(&map, neg_key) == GAME_WORLD_MAP_RESULT_OK, "create negative chunk");

    failed += assert_true(game_world_map_create_chunk(&map, neg_key) == GAME_WORLD_MAP_RESULT_CHUNK_EXISTS, "duplicate chunk exists");

    failed += assert_true(game_world_map_set(&map, left, 11) == GAME_WORLD_MAP_RESULT_OK, "set left in chunk");
    failed += assert_true(game_world_map_set(&map, right, 22) == GAME_WORLD_MAP_RESULT_OK, "set right in chunk");
    failed += assert_true(game_world_map_set(&map, neg, 33) == GAME_WORLD_MAP_RESULT_OK, "set negative in chunk");

    int32_t left_value = -999;
    int32_t right_value = -999;
    int32_t neg_value = -999;
    failed += assert_true(game_world_map_get(&map, left, &left_value) == GAME_WORLD_MAP_RESULT_OK, "get left");
    failed += assert_true(game_world_map_get(&map, right, &right_value) == GAME_WORLD_MAP_RESULT_OK, "get right");
    failed += assert_true(game_world_map_get(&map, neg, &neg_value) == GAME_WORLD_MAP_RESULT_OK, "get neg");
    failed += assert_true(left_value == 11, "left value equals 11");
    failed += assert_true(right_value == 22, "right value equals 22");
    failed += assert_true(neg_value == 33, "neg value equals 33");

    GameHexAxial missing = {2, 7};
    int32_t missing_value = 0;
    failed += assert_true(game_world_map_get(&map, missing, &missing_value) == GAME_WORLD_MAP_RESULT_CHUNK_MISSING, "missing chunk get");

    GameHexChunkLocal chunk_local = game_hex_cube_to_chunk_local(game_hex_axial_to_cube(left), 4);
    int32_t left_local_value = -1;
    failed += assert_true(game_world_map_get_local(&map, left_key, chunk_local, &left_local_value) == GAME_WORLD_MAP_RESULT_OK, "get local from explicit chunk/local");
    failed += assert_true(left_local_value == 11, "left local value equals 11");

    GameHexChunkLocal out_of_range = { -1, 0, 0 };
    int32_t dummy = 0;
    failed += assert_true(game_world_map_get_local(&map, left_key, out_of_range, &dummy) == GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT, "explicit local oob");

    WorldMapVisitState visit_state = {0};
    failed += assert_true(game_world_map_for_each_tile(&map, world_map_collect_visits, &visit_state) == GAME_WORLD_MAP_RESULT_OK, "iterate tiles");

    size_t expected_count = world_map_expected_chunk_tile_count(&map, left_key)
                            + world_map_expected_chunk_tile_count(&map, right_key)
                            + world_map_expected_chunk_tile_count(&map, neg_key);
    failed += assert_true(visit_state.callback_count == expected_count, "visited tile count for all chunks");
    failed += assert_true(visit_state.chunk_switches == 2u, "chunk iteration switches");
    failed += assert_true(visit_state.mismatch_count == 0u, "local iteration is stable in order");
    failed += assert_true(visit_state.chunk_order_ok, "sorted chunk order");

    game_world_map_destroy(&map);

    if (failed == 0) {
        printf("[world_map] PASS\n");
    }
    return failed;
}
