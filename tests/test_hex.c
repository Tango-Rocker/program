#include "world/hex.h"

#include <stdio.h>

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[hex] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int assert_equal_i32(int32_t lhs, int32_t rhs, const char *label) {
    if (lhs != rhs) {
        printf("[hex] FAIL: %s (%d != %d)\n", label, lhs, rhs);
        return 1;
    }
    return 0;
}

int test_hex(void) {
    int failed = 0;

    GameHexAxial axial = {3, -2};
    GameHexCube cube = game_hex_axial_to_cube(axial);
    GameHexAxial back = game_hex_cube_to_axial(cube);
    failed += assert_equal_i32(axial.q, back.q, "axial->cube->axial q");
    failed += assert_equal_i32(axial.r, back.r, "axial->cube->axial r");

    GameHexAxial neighbors[6] = {0};
    game_hex_axial_neighbors((GameHexAxial){1, 0}, neighbors);
    failed += assert_equal_i32(neighbors[0].q, 2, "neighbor 0 q");
    failed += assert_equal_i32(neighbors[0].r, 0, "neighbor 0 r");
    failed += assert_equal_i32(neighbors[1].q, 1, "neighbor 1 q");
    failed += assert_equal_i32(neighbors[1].r, 1, "neighbor 1 r");
    failed += assert_equal_i32(neighbors[2].q, 0, "neighbor 2 q");
    failed += assert_equal_i32(neighbors[2].r, 1, "neighbor 2 r");
    failed += assert_equal_i32(neighbors[3].q, 0, "neighbor 3 q");
    failed += assert_equal_i32(neighbors[3].r, 0, "neighbor 3 r");
    failed += assert_equal_i32(neighbors[4].q, 1, "neighbor 4 q");
    failed += assert_equal_i32(neighbors[4].r, -1, "neighbor 4 r");
    failed += assert_equal_i32(neighbors[5].q, 2, "neighbor 5 q");
    failed += assert_equal_i32(neighbors[5].r, -1, "neighbor 5 r");

    failed += assert_true(game_hex_axial_distance((GameHexAxial){0, 0}, (GameHexAxial){1, 0}) == 1,
                        "neighbor distance axial");
    failed += assert_true(game_hex_axial_distance((GameHexAxial){-2, 1}, (GameHexAxial){1, -1}) == 3,
                        "multi-step distance");

    GameHexCube chunk_cube = {3, 4, -7};
    GameHexChunkKey key = game_hex_cube_to_chunk_key(chunk_cube, 4);
    failed += assert_equal_i32(key.cx, 0, "chunk key positive cx");
    failed += assert_equal_i32(key.cy, 1, "chunk key positive cy");
    failed += assert_equal_i32(key.cz, -2, "chunk key negative cz");

    GameHexChunkLocal local = game_hex_cube_to_chunk_local(chunk_cube, 4);
    failed += assert_equal_i32(local.lx, 3, "chunk local x");
    failed += assert_equal_i32(local.ly, 0, "chunk local y");
    failed += assert_equal_i32(local.lz, 1, "chunk local z");
    failed += assert_true(game_hex_local_to_linear_index(local, 4) == 49u, "chunk local index 1");

    GameHexCube neg_cube = {-5, -7, 12};
    GameHexChunkKey neg_key = game_hex_cube_to_chunk_key(neg_cube, 4);
    failed += assert_equal_i32(neg_key.cx, -2, "negative chunk cx");
    failed += assert_equal_i32(neg_key.cy, -2, "negative chunk cy");
    failed += assert_equal_i32(neg_key.cz, 3, "negative chunk cz");

    GameHexChunkLocal neg_local = game_hex_cube_to_chunk_local(neg_cube, 4);
    failed += assert_equal_i32(neg_local.lx, 3, "negative local x");
    failed += assert_equal_i32(neg_local.ly, 1, "negative local y");
    failed += assert_equal_i32(neg_local.lz, 0, "negative local z");

    if (failed == 0) {
        printf("[hex] PASS\n");
    }

    return failed;
}
