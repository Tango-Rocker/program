#include "nav/pathfind.h"

#include <limits.h>
#include <stddef.h>

size_t game_pathfind_required_capacity(const GamePathGrid *grid) {
    if (!grid || grid->q_count == 0u || grid->r_count == 0u) {
        return 0u;
    }

    if (grid->q_count > (SIZE_MAX / sizeof(size_t)) / grid->r_count) {
        return 0u;
    }

    return grid->q_count * grid->r_count;
}

size_t game_pathfind_index(const GamePathCostMap *map, GameHexAxial tile) {
    if (!map || !map->cost) {
        return (size_t)-1;
    }

    int64_t q_offset = (int64_t)tile.q - map->grid.q_min;
    int64_t r_offset = (int64_t)tile.r - map->grid.r_min;
    if (q_offset < 0 || r_offset < 0) {
        return (size_t)-1;
    }

    if (q_offset >= (int64_t)map->grid.q_count || r_offset >= (int64_t)map->grid.r_count) {
        return (size_t)-1;
    }

    return (size_t)(q_offset * (int64_t)map->grid.r_count + r_offset);
}

bool game_pathfind_in_grid(const GamePathGrid *grid, GameHexAxial tile) {
    if (!grid) {
        return false;
    }

    if (grid->q_count == 0u || grid->r_count == 0u) {
        return false;
    }

    int64_t q_offset = (int64_t)tile.q - (int64_t)grid->q_min;
    int64_t r_offset = (int64_t)tile.r - (int64_t)grid->r_min;

    return !(q_offset < 0 || r_offset < 0 || q_offset >= (int64_t)grid->q_count || r_offset >= (int64_t)grid->r_count);
}

static int32_t game_pathfind_normalize_cost(const GamePathCostMap *map, size_t index) {
    if (!map || !map->cost || index >= game_pathfind_required_capacity(&map->grid)) {
        return -1;
    }

    return (int32_t)map->cost[index];
}

int32_t game_pathfind_cost(const GamePathCostMap *map, GameHexAxial tile) {
    if (!map || !map->cost) {
        return -1;
    }

    size_t index = game_pathfind_index(map, tile);
    if (index == (size_t)-1) {
        return -1;
    }

    return game_pathfind_normalize_cost(map, index);
}

static bool game_pathfind_is_passable(const GamePathCostMap *map, GameHexAxial tile) {
    int32_t cost = game_pathfind_cost(map, tile);
    return cost >= 0 && cost != (int32_t)map->blocked_cost;
}

static GamePathFindResult game_pathfind_restart_scratch(const GamePathCostMap *map, const GamePathQueryScratch *scratch) {
    if (!map || !map->cost || !scratch || !scratch->g_score || !scratch->f_score || !scratch->parent || !scratch->open || !scratch->closed) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    size_t capacity = game_pathfind_required_capacity(&map->grid);
    if (capacity == 0u || capacity != scratch->capacity) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < capacity; ++i) {
        scratch->g_score[i] = UINT32_MAX;
        scratch->f_score[i] = UINT32_MAX;
        scratch->parent[i] = -1;
        scratch->open[i] = false;
        scratch->closed[i] = false;
    }

    return GAME_PATH_FIND_RESULT_OK;
}

static size_t game_pathfind_capacity_or_zero(const GamePathCostMap *map) {
    return (map && map->grid.q_count && map->grid.r_count) ? game_pathfind_required_capacity(&map->grid) : 0u;
}

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
) {
    if (!out_path_length || !out_expanded_count) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    *out_path_length = 0u;
    *out_expanded_count = 0u;

    if (!cost_map || !cost_map->cost || !scratch || !out_path) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    if (!game_pathfind_in_grid(&cost_map->grid, start) || !game_pathfind_in_grid(&cost_map->grid, goal)) {
        return GAME_PATH_FIND_RESULT_OUT_OF_BOUNDS;
    }

    if (!game_pathfind_is_passable(cost_map, start) || !game_pathfind_is_passable(cost_map, goal)) {
        return GAME_PATH_FIND_RESULT_OUT_OF_BOUNDS;
    }

    size_t capacity = game_pathfind_capacity_or_zero(cost_map);
    if (capacity == 0u) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    if (scratch->capacity != capacity) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    if (out_path_capacity == 0u) {
        return GAME_PATH_FIND_RESULT_PATH_TOO_SMALL;
    }

    GamePathFindResult result = game_pathfind_restart_scratch(cost_map, scratch);
    if (result != GAME_PATH_FIND_RESULT_OK) {
        return result;
    }

    if (start.q == goal.q && start.r == goal.r) {
        if (out_path_capacity < 1u) {
            return GAME_PATH_FIND_RESULT_PATH_TOO_SMALL;
        }
        out_path[0] = start;
        *out_path_length = 1u;
        return GAME_PATH_FIND_RESULT_OK;
    }

    if (node_budget == 0u) {
        return GAME_PATH_FIND_RESULT_BUDGET_EXHAUSTED;
    }

    size_t start_index = game_pathfind_index(cost_map, start);
    size_t goal_index = game_pathfind_index(cost_map, goal);

    scratch->g_score[start_index] = 0u;
    scratch->f_score[start_index] = (uint32_t)game_hex_axial_distance(start, goal);
    scratch->open[start_index] = true;

    size_t expansions = 0u;
    bool found = false;

    while (1) {
        size_t best_index = (size_t)-1;
        uint32_t best_f = UINT32_MAX;
        uint32_t best_g = UINT32_MAX;
        GameHexAxial best_tile = {0};

        for (size_t i = 0u; i < capacity; ++i) {
            if (!scratch->open[i]) {
                continue;
            }

            if (scratch->f_score[i] < best_f) {
                best_f = scratch->f_score[i];
                best_g = scratch->g_score[i];
                best_index = i;
                best_tile = (GameHexAxial){
                    .q = cost_map->grid.q_min + (int32_t)(i / cost_map->grid.r_count),
                    .r = cost_map->grid.r_min + (int32_t)(i % cost_map->grid.r_count),
                };
            } else if (scratch->f_score[i] == best_f && scratch->g_score[i] < best_g) {
                best_f = scratch->f_score[i];
                best_g = scratch->g_score[i];
                best_index = i;
                best_tile = (GameHexAxial){
                    .q = cost_map->grid.q_min + (int32_t)(i / cost_map->grid.r_count),
                    .r = cost_map->grid.r_min + (int32_t)(i % cost_map->grid.r_count),
                };
            } else if (scratch->f_score[i] == best_f && scratch->g_score[i] == best_g) {
                GameHexAxial tile = {
                    .q = cost_map->grid.q_min + (int32_t)(i / cost_map->grid.r_count),
                    .r = cost_map->grid.r_min + (int32_t)(i % cost_map->grid.r_count),
                };

                if (best_index == (size_t)-1) {
                    best_index = i;
                    best_tile = tile;
                } else if (tile.q > best_tile.q || (tile.q == best_tile.q && tile.r < best_tile.r)) {
                    best_index = i;
                    best_tile = tile;
                }
            }
        }

        if (best_index == (size_t)-1) {
            break;
        }

        scratch->open[best_index] = false;
        scratch->closed[best_index] = true;
        expansions++;
        *out_expanded_count = expansions;

        if (best_index == goal_index) {
            found = true;
            break;
        }

        if (expansions >= node_budget) {
            return GAME_PATH_FIND_RESULT_BUDGET_EXHAUSTED;
        }

        GameHexAxial best = {
            .q = cost_map->grid.q_min + (int32_t)(best_index / cost_map->grid.r_count),
            .r = cost_map->grid.r_min + (int32_t)(best_index % cost_map->grid.r_count),
        };

        GameHexAxial neighbors[6];
        game_hex_axial_neighbors(best, neighbors);
        uint32_t best_g_score = scratch->g_score[best_index];
        for (int n = 0; n < 6; ++n) {
            GameHexAxial next = neighbors[n];
            if (!game_pathfind_in_grid(&cost_map->grid, next)) {
                continue;
            }
            if (!game_pathfind_is_passable(cost_map, next)) {
                continue;
            }

            size_t next_index = game_pathfind_index(cost_map, next);
            if (scratch->closed[next_index]) {
                continue;
            }

            int32_t move_cost = game_pathfind_cost(cost_map, next);
            if (move_cost < 0) {
                continue;
            }

            uint64_t tentative = (uint64_t)best_g_score + (uint64_t)(uint32_t)move_cost;
            if (tentative > UINT32_MAX) {
                return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
            }

            uint32_t tentative_g = (uint32_t)tentative;
            if (tentative_g < scratch->g_score[next_index]) {
                scratch->parent[next_index] = (int32_t)best_index;
                scratch->g_score[next_index] = tentative_g;
                scratch->f_score[next_index] = tentative_g + (uint32_t)game_hex_axial_distance(next, goal);
                scratch->open[next_index] = true;
            }
        }
    }

    if (!found) {
        return GAME_PATH_FIND_RESULT_NO_ROUTE;
    }

    size_t path_length = 0u;
    int32_t cursor = (int32_t)goal_index;
    while (cursor >= 0) {
        if (path_length >= out_path_capacity) {
            return GAME_PATH_FIND_RESULT_PATH_TOO_SMALL;
        }
        out_path[path_length++] = (GameHexAxial){
            .q = cost_map->grid.q_min + (int32_t)(cursor / (int32_t)cost_map->grid.r_count),
            .r = cost_map->grid.r_min + (int32_t)(cursor % cost_map->grid.r_count),
        };
        cursor = scratch->parent[cursor];
    }

    for (size_t i = 0u; i < path_length / 2u; ++i) {
        size_t other = path_length - 1u - i;
        GameHexAxial tmp = out_path[i];
        out_path[i] = out_path[other];
        out_path[other] = tmp;
    }

    *out_path_length = path_length;
    return GAME_PATH_FIND_RESULT_OK;
}
