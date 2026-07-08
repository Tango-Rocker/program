#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "nav/pathfind.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[pathfind] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_pathfind(void) {
    int failed = 0;
    GameHexAxial path[16];
    size_t path_len = 0u;
    size_t expanded = 0u;

    uint32_t g1[5] = {0};
    uint32_t f1[5] = {0};
    int32_t parent1[5] = {0};
    bool open1[5] = {0};
    bool closed1[5] = {0};
    bool touched_flags1[5] = {0};
    size_t heap1[5] = {0};
    size_t heap_pos1[5] = {0};
    size_t touched1[5] = {0};
    GamePathQueryScratch scratch1 = {
        .capacity = 5u,
        .g_score = g1,
        .f_score = f1,
        .parent = parent1,
        .open = open1,
        .closed = closed1,
        .touched_flags = touched_flags1,
        .heap = heap1,
        .heap_pos = heap_pos1,
        .touched = touched1,
        .heap_capacity = 5u,
        .heap_pos_capacity = 5u,
        .touched_capacity = 5u,
    };
    uint16_t map1[5] = {1, 1, 1, 1, 1};
    GamePathCostMap straight = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 1u, .r_count = 5u},
        .cost = map1,
        .blocked_cost = UINT16_MAX,
    };

    failed += assert_true(
        game_pathfind_query((GameHexAxial){0, 0}, (GameHexAxial){0, 4}, &straight, &scratch1, 10u, path, 16u, &path_len, &expanded)
            == GAME_PATH_FIND_RESULT_OK,
        "straight route success"
    );
    failed += assert_true(path_len == 5u, "straight path len 5");
    failed += assert_true(path[0].q == 0 && path[0].r == 0, "straight start");
    failed += assert_true(path[4].q == 0 && path[4].r == 4, "straight goal");

    uint32_t g2[9] = {0};
    uint32_t f2[9] = {0};
    int32_t parent2[9] = {0};
    bool open2[9] = {0};
    bool closed2[9] = {0};
    bool touched_flags2[9] = {0};
    size_t heap2[9] = {0};
    size_t heap_pos2[9] = {0};
    size_t touched2[9] = {0};
    GamePathQueryScratch scratch2 = {
        .capacity = 9u,
        .g_score = g2,
        .f_score = f2,
        .parent = parent2,
        .open = open2,
        .closed = closed2,
        .touched_flags = touched_flags2,
        .heap = heap2,
        .heap_pos = heap_pos2,
        .touched = touched2,
        .heap_capacity = 9u,
        .heap_pos_capacity = 9u,
        .touched_capacity = 9u,
    };
    uint16_t map2[9] = {
        1, 1, 1,
        1, UINT16_MAX, 1,
        1, 1, 1,
    };
    GamePathCostMap detour = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 3u, .r_count = 3u},
        .cost = map2,
        .blocked_cost = UINT16_MAX,
    };
    failed += assert_true(
        game_pathfind_query((GameHexAxial){0, 0}, (GameHexAxial){2, 0}, &detour, &scratch2, 20u, path, 16u, &path_len, &expanded)
            == GAME_PATH_FIND_RESULT_OK,
        "detour route success"
    );
    failed += assert_true(path_len > 0u, "detour path non-empty");

    uint32_t g3[4] = {0};
    uint32_t f3[4] = {0};
    int32_t parent3[4] = {0};
    bool open3[4] = {0};
    bool closed3[4] = {0};
    bool touched_flags3[4] = {0};
    size_t heap3[4] = {0};
    size_t heap_pos3[4] = {0};
    size_t touched3[4] = {0};
    GamePathQueryScratch scratch3 = {
        .capacity = 4u,
        .g_score = g3,
        .f_score = f3,
        .parent = parent3,
        .open = open3,
        .closed = closed3,
        .touched_flags = touched_flags3,
        .heap = heap3,
        .heap_pos = heap_pos3,
        .touched = touched3,
        .heap_capacity = 4u,
        .heap_pos_capacity = 4u,
        .touched_capacity = 4u,
    };
    uint16_t map3[4] = {
        1, UINT16_MAX,
        UINT16_MAX, 1,
    };
    GamePathCostMap no_route = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 2u, .r_count = 2u},
        .cost = map3,
        .blocked_cost = UINT16_MAX,
    };
    failed += assert_true(
        game_pathfind_query((GameHexAxial){0, 0}, (GameHexAxial){1, 1}, &no_route, &scratch3, 12u, path, 16u, &path_len, &expanded)
            == GAME_PATH_FIND_RESULT_NO_ROUTE,
        "unreachable goal returns no route"
    );

    failed += assert_true(
        game_pathfind_query((GameHexAxial){0, 0}, (GameHexAxial){0, 4}, &straight, &scratch1, 2u, path, 16u, &path_len, &expanded)
            == GAME_PATH_FIND_RESULT_BUDGET_EXHAUSTED,
        "budget exhaustion reported"
    );

    uint32_t g4[4] = {0};
    uint32_t f4[4] = {0};
    int32_t parent4[4] = {0};
    bool open4[4] = {0};
    bool closed4[4] = {0};
    bool touched_flags4[4] = {0};
    size_t heap4[4] = {0};
    size_t heap_pos4[4] = {0};
    size_t touched4[4] = {0};
    GamePathQueryScratch scratch4 = {
        .capacity = 4u,
        .g_score = g4,
        .f_score = f4,
        .parent = parent4,
        .open = open4,
        .closed = closed4,
        .touched_flags = touched_flags4,
        .heap = heap4,
        .heap_pos = heap_pos4,
        .touched = touched4,
        .heap_capacity = 4u,
        .heap_pos_capacity = 4u,
        .touched_capacity = 4u,
    };
    uint16_t map4[4] = {1, 1, 1, 1};
    GamePathCostMap tie = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 2u, .r_count = 2u},
        .cost = map4,
        .blocked_cost = UINT16_MAX,
    };
    failed += assert_true(
        game_pathfind_query((GameHexAxial){0, 0}, (GameHexAxial){1, 1}, &tie, &scratch4, 20u, path, 16u, &path_len, &expanded)
            == GAME_PATH_FIND_RESULT_OK,
        "tie-break route exists"
    );
    failed += assert_true(path_len == 3u, "tie-break path length");
    failed += assert_true(path[1].q == 1 && path[1].r == 0, "tie-break picks first-neighbor path");

    uint16_t map5[5] = {1, 1, 1, 1, 1};
    GamePathCostMap tiny = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 1u, .r_count = 5u},
        .cost = map5,
        .blocked_cost = UINT16_MAX,
    };
    failed += assert_true(
        game_pathfind_query((GameHexAxial){0, 0}, (GameHexAxial){0, 4}, &tiny, &scratch1, 10u, path, 1u, &path_len, &expanded)
            == GAME_PATH_FIND_RESULT_PATH_TOO_SMALL,
        "path too small fails"
    );

    enum { LARGE_Q = 64, LARGE_R = 64, LARGE_CAPACITY = LARGE_Q * LARGE_R };
    static uint16_t large_map[LARGE_CAPACITY];
    static uint32_t large_g[LARGE_CAPACITY];
    static uint32_t large_f[LARGE_CAPACITY];
    static int32_t large_parent[LARGE_CAPACITY];
    static bool large_open[LARGE_CAPACITY];
    static bool large_closed[LARGE_CAPACITY];
    static bool large_touched_flags[LARGE_CAPACITY];
    static size_t large_heap[LARGE_CAPACITY];
    static size_t large_heap_pos[LARGE_CAPACITY];
    static size_t large_touched[LARGE_CAPACITY];
    for (size_t i = 0u; i < LARGE_CAPACITY; ++i) {
        large_map[i] = 1u;
    }
    GamePathCostMap large = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = LARGE_Q, .r_count = LARGE_R},
        .cost = large_map,
        .blocked_cost = UINT16_MAX,
    };
    GamePathQueryScratch large_scratch = {
        .capacity = LARGE_CAPACITY,
        .g_score = large_g,
        .f_score = large_f,
        .parent = large_parent,
        .open = large_open,
        .closed = large_closed,
        .touched_flags = large_touched_flags,
        .heap = large_heap,
        .heap_pos = large_heap_pos,
        .touched = large_touched,
        .heap_capacity = LARGE_CAPACITY,
        .heap_pos_capacity = LARGE_CAPACITY,
        .touched_capacity = LARGE_CAPACITY,
    };
    GameHexAxial large_path[160];
    failed += assert_true(
        game_pathfind_query((GameHexAxial){0, 0}, (GameHexAxial){63, 63}, &large, &large_scratch, LARGE_CAPACITY,
                            large_path, 160u, &path_len, &expanded) == GAME_PATH_FIND_RESULT_OK,
        "large heap route succeeds"
    );
    failed += assert_true(path_len == 127u, "large heap route length");
    failed += assert_true(expanded > 0u && expanded <= LARGE_CAPACITY, "large heap reports bounded expansion count");

    if (failed == 0) {
        printf("[pathfind] PASS\n");
    }
    return failed;
}
