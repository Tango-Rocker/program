#include "world/scenario_gen.h"

#include <string.h>

#include "core/random.h"
#include "world/topology.h"

static bool game_scenario_is_passable(int32_t tile_value) {
    return tile_value >= 0;
}

static GameWorldMapResult game_scenario_ensure_chunk(GameWorldMap *map, GameHexAxial tile) {
    GameHexChunkKey key = game_hex_cube_to_chunk_key(game_hex_axial_to_cube(tile), map->chunk_radius);
    GameWorldMapResult result = game_world_map_create_chunk(map, key);
    if (result == GAME_WORLD_MAP_RESULT_CHUNK_EXISTS) {
        return GAME_WORLD_MAP_RESULT_OK;
    }
    return result;
}

void game_scenario_destroy(GameScenario *scenario) {
    if (!scenario) {
        return;
    }
    game_world_map_destroy(&scenario->map);
    *scenario = (GameScenario){0};
}

GameScenarioGenResult game_scenario_generate(GameScenario *scenario, const GameScenarioConfig *config) {
    if (!scenario || !config || config->width <= 0 || config->height <= 0 || config->width > 2048 ||
        config->height > 2048 || config->block_chance_percent > 95u) {
        return GAME_SCENARIO_GEN_RESULT_INVALID_ARGUMENT;
    }

    *scenario = (GameScenario){0};
    if (game_world_map_init(&scenario->map, GAME_HEX_CHUNK_RADIUS, -1) != GAME_WORLD_MAP_RESULT_OK) {
        return GAME_SCENARIO_GEN_RESULT_INVALID_ARGUMENT;
    }

    GameRng rng = {0};
    game_rng_seed(&rng, config->seed);
    scenario->party_anchor = (GameHexAxial){0, 0};
    scenario->colony_anchor = (GameHexAxial){config->width / 2, config->height / 2};
    scenario->horde_anchor = (GameHexAxial){config->width - 1, config->height - 1};
    scenario->stockpile_anchor = scenario->colony_anchor;
    scenario->has_stockpile = config->include_stockpile;

    for (int32_t r = 0; r < config->height; ++r) {
        for (int32_t q = 0; q < config->width; ++q) {
            GameHexAxial tile = {q, r};
            if (game_scenario_ensure_chunk(&scenario->map, tile) != GAME_WORLD_MAP_RESULT_OK) {
                game_scenario_destroy(scenario);
                return GAME_SCENARIO_GEN_RESULT_OUT_OF_MEMORY;
            }

            bool anchor = (q == scenario->party_anchor.q && r == scenario->party_anchor.r)
                          || (q == scenario->colony_anchor.q && r == scenario->colony_anchor.r)
                          || (q == scenario->horde_anchor.q && r == scenario->horde_anchor.r);
            uint32_t roll = game_rng_u32(&rng) % 100u;
            int32_t value = (!anchor && roll < config->block_chance_percent) ? -1 : 1;
            if (game_world_map_set(&scenario->map, tile, value) != GAME_WORLD_MAP_RESULT_OK) {
                game_scenario_destroy(scenario);
                return GAME_SCENARIO_GEN_RESULT_OUT_OF_MEMORY;
            }
            if (value >= 0) {
                ++scenario->passable_tiles;
            } else {
                ++scenario->blocked_tiles;
            }
        }
    }

    for (int32_t q = scenario->party_anchor.q; q <= scenario->colony_anchor.q; ++q) {
        int32_t previous = 0;
        GameHexAxial tile = {q, scenario->party_anchor.r};
        if (game_world_map_get(&scenario->map, tile, &previous) == GAME_WORLD_MAP_RESULT_OK && previous < 0) {
            (void)game_world_map_set(&scenario->map, tile, 1);
            --scenario->blocked_tiles;
            ++scenario->passable_tiles;
        }
    }
    for (int32_t r = scenario->party_anchor.r; r <= scenario->colony_anchor.r; ++r) {
        int32_t previous = 0;
        GameHexAxial tile = {scenario->colony_anchor.q, r};
        if (game_world_map_get(&scenario->map, tile, &previous) == GAME_WORLD_MAP_RESULT_OK && previous < 0) {
            (void)game_world_map_set(&scenario->map, tile, 1);
            --scenario->blocked_tiles;
            ++scenario->passable_tiles;
        }
    }

    GameWorldTopology topology = {0};
    game_world_topology_init(&topology);
    if (game_world_topology_rebuild(&topology, &scenario->map, game_scenario_is_passable, NULL)
        != GAME_WORLD_TOPOLOGY_RESULT_OK) {
        game_world_topology_destroy(&topology);
        game_scenario_destroy(scenario);
        return GAME_SCENARIO_GEN_RESULT_TOPOLOGY_FAILED;
    }

    bool sane = game_world_topology_same_region(&topology, scenario->party_anchor, scenario->colony_anchor)
                && scenario->passable_tiles > 0u;
    game_world_topology_destroy(&topology);
    if (!sane) {
        game_scenario_destroy(scenario);
        return GAME_SCENARIO_GEN_RESULT_TOPOLOGY_FAILED;
    }

    return GAME_SCENARIO_GEN_RESULT_OK;
}
