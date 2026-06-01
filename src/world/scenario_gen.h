#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "world/hex.h"
#include "world/world_map.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_SCENARIO_GEN_RESULT_OK = 0,
    GAME_SCENARIO_GEN_RESULT_INVALID_ARGUMENT = 1,
    GAME_SCENARIO_GEN_RESULT_OUT_OF_MEMORY = 2,
    GAME_SCENARIO_GEN_RESULT_TOPOLOGY_FAILED = 3,
} GameScenarioGenResult;

typedef struct {
    uint64_t seed;
    int32_t width;
    int32_t height;
    uint32_t block_chance_percent;
    bool include_stockpile;
} GameScenarioConfig;

typedef struct {
    GameWorldMap map;
    GameHexAxial party_anchor;
    GameHexAxial horde_anchor;
    GameHexAxial colony_anchor;
    GameHexAxial stockpile_anchor;
    bool has_stockpile;
    uint32_t passable_tiles;
    uint32_t blocked_tiles;
} GameScenario;

GameScenarioGenResult game_scenario_generate(GameScenario *scenario, const GameScenarioConfig *config);
void game_scenario_destroy(GameScenario *scenario);

#ifdef __cplusplus
}
#endif
