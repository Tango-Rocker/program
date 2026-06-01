#include <stdio.h>

#include "world/hex.h"
#include "world/structure.h"
#include "world/world_map.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[structure] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int test_structure_placement_blocking_and_removal(void) {
    int failed = 0;

    GameWorldMap map = {0};
    failed += assert_true(
        game_world_map_init(&map, 4, -1) == GAME_WORLD_MAP_RESULT_OK,
        "structure: init map for placement"
    );

    static const GameStructureFootprintOffset wall_footprint[4] = {
        {1, 0},
        {0, 1},
        {0, 0},
        {1, 1},
    };
    static const GameStructureDefinition definitions[] = {
        {11u, GAME_STRUCTURE_TILE_FLAG_BLOCKS_PATH, GAME_STRUCTURE_FIELD_FLAG_NONE, 4u, wall_footprint},
    };

    GameStructurePlacement placements[4] = {0};
    GameStructureSystem structure_system = {0};
    failed += assert_true(
        game_structure_init(
            &structure_system,
            definitions,
            1u,
            placements,
            4u,
            1u
        ) == GAME_STRUCTURE_RESULT_OK,
        "structure: initialize structure system"
    );

    GameHexAxial anchor = {2, 2};
    GameHexChunkKey chunk_key = game_hex_cube_to_chunk_key(game_hex_axial_to_cube(anchor), 4);
    failed += assert_true(
        game_world_map_create_chunk(&map, chunk_key) == GAME_WORLD_MAP_RESULT_OK,
        "structure: create chunk for placement area"
    );

    GameHexAxial footprint_tiles[4] = {{2, 2}, {3, 2}, {2, 3}, {3, 3}};
    for (size_t i = 0u; i < 4u; ++i) {
        GameHexChunkKey footprint_chunk = game_hex_cube_to_chunk_key(game_hex_axial_to_cube(footprint_tiles[i]), 4);
        GameWorldMapResult create_result = game_world_map_create_chunk(&map, footprint_chunk);
        failed += assert_true(
            create_result == GAME_WORLD_MAP_RESULT_OK || create_result == GAME_WORLD_MAP_RESULT_CHUNK_EXISTS,
            "structure: create chunk for footprint tile"
        );
    }
    failed += assert_true(
        game_world_map_set(&map, footprint_tiles[0], 7) == GAME_WORLD_MAP_RESULT_OK,
        "structure: set baseline map value for first footprint tile"
    );
    failed += assert_true(
        game_world_map_set(&map, footprint_tiles[1], 11) == GAME_WORLD_MAP_RESULT_OK,
        "structure: set baseline map value for second footprint tile"
    );
    failed += assert_true(
        game_world_map_set(&map, footprint_tiles[2], 13) == GAME_WORLD_MAP_RESULT_OK,
        "structure: set baseline map value for third footprint tile"
    );
    failed += assert_true(
        game_world_map_set(&map, footprint_tiles[3], 17) == GAME_WORLD_MAP_RESULT_OK,
        "structure: set baseline map value for fourth footprint tile"
    );

    failed += assert_true(
        !game_structure_is_topology_dirty(&structure_system),
        "structure: topology dirty flag starts clear"
    );

    GameStructurePlacementHandle placed = {0u, 0u};
    failed += assert_true(
        game_structure_validate_placement(&structure_system, &map, 11u, anchor, false) == GAME_STRUCTURE_RESULT_OK,
        "structure: validate first placement"
    );
    failed += assert_true(
        game_structure_place(&structure_system, &map, 11u, anchor, &placed) == GAME_STRUCTURE_RESULT_OK,
        "structure: place blocking structure"
    );
    failed += assert_true(game_structure_is_topology_dirty(&structure_system), "structure: topology dirty after placement");

    for (size_t i = 0u; i < 4u; ++i) {
        int32_t placed_value = 0;
        failed += assert_true(
            game_world_map_get(&map, footprint_tiles[i], &placed_value) == GAME_WORLD_MAP_RESULT_OK,
            "structure: placed tile is queryable"
        );
        failed += assert_true(game_structure_tile_has_structure(placed_value), "structure: tile encoded with structure payload");
        failed += assert_true(game_structure_tile_blocks_path(placed_value), "structure: tile blocks path");
    }

    failed += assert_true(
        game_structure_place(&structure_system, &map, 11u, anchor, &(GameStructurePlacementHandle){0u, 0u})
        == GAME_STRUCTURE_RESULT_ALREADY_USED,
        "structure: placement is rejected for overlapping footprint"
    );

    failed += assert_true(
        game_structure_remove(&structure_system, &map, placed) == GAME_STRUCTURE_RESULT_OK,
        "structure: remove placed structure"
    );
    failed += assert_true(game_structure_is_topology_dirty(&structure_system), "structure: topology dirty after removal");

    failed += assert_true(
        game_structure_status(&structure_system, placed, &(GameStructurePlacement){0}) == GAME_STRUCTURE_RESULT_INVALID_ARGUMENT,
        "structure: old placement handle becomes invalid after removal"
    );

    for (size_t i = 0u; i < 4u; ++i) {
        int32_t restored = 0;
        failed += assert_true(
            game_world_map_get(&map, footprint_tiles[i], &restored) == GAME_WORLD_MAP_RESULT_OK,
            "structure: removed tile remains queryable"
        );
        failed += assert_true(
            restored == (int32_t[]){7, 11, 13, 17}[i],
            "structure: removed tile restores previous terrain"
        );
    }

    game_structure_mark_topology_clean(&structure_system);
    failed += assert_true(
        !game_structure_is_topology_dirty(&structure_system),
        "structure: topology dirty flag can be cleared"
    );

    game_world_map_destroy(&map);
    return failed;
}

static int test_structure_footprint_sorting(void) {
    int failed = 0;

    static const GameStructureFootprintOffset shuffled_footprint[4] = {
        {2, 0},
        {-1, 1},
        {0, -2},
        {0, 0},
    };
    static const GameStructureDefinition definition = {
        22u,
        0u,
        GAME_STRUCTURE_FIELD_FLAG_NONE,
        4u,
        shuffled_footprint,
    };

    GameHexAxial anchor = {1, 1};
    GameHexAxial footprint_tiles[4] = {0};
    size_t count = game_structure_compute_footprint_tiles(
        &definition,
        anchor,
        footprint_tiles,
        4u
    );

    static const GameHexAxial expected_order[4] = {
        {0, 2},
        {1, -1},
        {1, 1},
        {3, 1},
    };

    failed += assert_true(count == 4u, "structure: compute footprint returns full deterministic count");
    for (size_t i = 0u; i < 4u; ++i) {
        failed += assert_true(
            footprint_tiles[i].q == expected_order[i].q && footprint_tiles[i].r == expected_order[i].r,
            "structure: deterministic footprint sort is stable and ordered"
        );
    }

    return failed;
}

int test_structure(void) {
    int failed = 0;
    failed += test_structure_placement_blocking_and_removal();
    failed += test_structure_footprint_sorting();

    if (failed == 0) {
        printf("[structure] PASS\n");
    }
    return failed;
}
