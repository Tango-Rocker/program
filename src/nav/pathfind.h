#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GamePathGrid {
    int32_t q_min;
    int32_t r_min;
    size_t q_count;
    size_t r_count;
} GamePathGrid;

typedef struct GamePathCostMap {
    GamePathGrid grid;
    const uint16_t *cost;
    uint16_t blocked_cost;
} GamePathCostMap;

typedef struct GamePathQueryScratch {
    size_t capacity;
    uint32_t *g_score;
    uint32_t *f_score;
    int32_t *parent;
    bool *open;
    bool *closed;
} GamePathQueryScratch;

typedef enum {
    GAME_PATH_FIND_RESULT_OK = 0,
    GAME_PATH_FIND_RESULT_INVALID_ARGUMENT = 1,
    GAME_PATH_FIND_RESULT_OUT_OF_BOUNDS = 2,
    GAME_PATH_FIND_RESULT_NO_ROUTE = 3,
    GAME_PATH_FIND_RESULT_BUDGET_EXHAUSTED = 4,
    GAME_PATH_FIND_RESULT_PATH_TOO_SMALL = 5,
} GamePathFindResult;

size_t game_pathfind_required_capacity(const GamePathGrid *grid);
size_t game_pathfind_index(const GamePathCostMap *map, GameHexAxial tile);
bool game_pathfind_in_grid(const GamePathGrid *grid, GameHexAxial tile);
int32_t game_pathfind_cost(const GamePathCostMap *map, GameHexAxial tile);

GamePathFindResult game_pathfind_query(
    GameHexAxial start,
    GameHexAxial goal,
    const GamePathCostMap *cost_map,
    const GamePathQueryScratch *scratch,
    uint32_t node_budget,
    GameHexAxial *out_path,
    size_t out_path_capacity,
    size_t *out_path_length,
    size_t *out_expanded_count
);

#ifdef __cplusplus
}
#endif
