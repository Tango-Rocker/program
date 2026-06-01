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
    GamePathQueryScratch scratch1 = {
        .capacity = 5u,
        .g_score = g1,
        .f_score = f1,
        .parent = parent1,
        .open = open1,
        .closed = closed1,
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
    GamePathQueryScratch scratch2 = {
        .capacity = 9u,
        .g_score = g2,
        .f_score = f2,
        .parent = parent2,
        .open = open2,
        .closed = closed2,
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
    GamePathQueryScratch scratch3 = {
        .capacity = 4u,
        .g_score = g3,
        .f_score = f3,
        .parent = parent3,
        .open = open3,
        .closed = closed3,
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
    GamePathQueryScratch scratch4 = {
        .capacity = 4u,
        .g_score = g4,
        .f_score = f4,
        .parent = parent4,
        .open = open4,
        .closed = closed4,
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

    if (failed == 0) {
        printf("[pathfind] PASS\n");
    }
    return failed;
}
