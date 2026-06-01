#include <stdio.h>

#include "world/scenario_gen.h"
#include "world/topology.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[scenario_gen] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static bool test_scenario_passable(int32_t value) {
    return value >= 0;
}

int test_scenario_gen(void) {
    int failed = 0;
    GameScenarioConfig config = {
        .seed = 77u,
        .width = 5,
        .height = 5,
        .block_chance_percent = 0u,
        .include_stockpile = true,
    };
    GameScenario first = {0};
    GameScenario second = {0};
    failed += assert_true(game_scenario_generate(&first, &config) == GAME_SCENARIO_GEN_RESULT_OK, "generate first");
    failed += assert_true(game_scenario_generate(&second, &config) == GAME_SCENARIO_GEN_RESULT_OK, "generate second");
    failed += assert_true(first.passable_tiles == second.passable_tiles, "same seed stable passable count");
    failed += assert_true(first.party_anchor.q == 0 && first.colony_anchor.q == 2, "anchors stable");
    failed += assert_true(first.has_stockpile, "stockpile included");

    int32_t first_value = 0;
    int32_t second_value = 0;
    failed += assert_true(game_world_map_get(&first.map, (GameHexAxial){2, 2}, &first_value) == GAME_WORLD_MAP_RESULT_OK,
                          "read first map");
    failed += assert_true(game_world_map_get(&second.map, (GameHexAxial){2, 2}, &second_value) == GAME_WORLD_MAP_RESULT_OK,
                          "read second map");
    failed += assert_true(first_value == second_value, "same seed stable tile");

    GameScenario different = {0};
    config.seed = 78u;
    config.block_chance_percent = 40u;
    failed += assert_true(game_scenario_generate(&different, &config) == GAME_SCENARIO_GEN_RESULT_OK, "generate different");
    failed += assert_true(different.blocked_tiles != first.blocked_tiles, "different seed/config changes output");

    GameWorldTopology topology = {0};
    game_world_topology_init(&topology);
    failed += assert_true(
        game_world_topology_rebuild(&topology, &first.map, test_scenario_passable, NULL)
            == GAME_WORLD_TOPOLOGY_RESULT_OK,
        "topology rebuild"
    );
    failed += assert_true(
        game_world_topology_same_region(&topology, first.party_anchor, first.colony_anchor),
        "generated map topology sane"
    );
    game_world_topology_destroy(&topology);

    GameScenario invalid = {0};
    GameScenarioConfig bad = {.seed = 1u, .width = 0, .height = 5};
    failed += assert_true(game_scenario_generate(&invalid, &bad) == GAME_SCENARIO_GEN_RESULT_INVALID_ARGUMENT,
                          "invalid config rejected");

    game_scenario_destroy(&first);
    game_scenario_destroy(&second);
    game_scenario_destroy(&different);
    if (failed == 0) {
        printf("[scenario_gen] PASS\n");
    }
    return failed;
}
