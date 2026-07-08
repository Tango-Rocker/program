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

static GamePathFindResult game_pathfind_restart_scratch(const GamePathCostMap *map, GamePathQueryScratch *scratch) {
    if (!map || !map->cost || !scratch || !scratch->g_score || !scratch->f_score || !scratch->parent ||
        !scratch->open || !scratch->closed || !scratch->touched_flags || !scratch->heap || !scratch->heap_pos ||
        !scratch->touched) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    size_t capacity = game_pathfind_required_capacity(&map->grid);
    if (capacity == 0u || capacity > scratch->capacity || scratch->heap_capacity < scratch->capacity ||
        scratch->heap_pos_capacity < scratch->capacity || scratch->touched_capacity < scratch->capacity) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < scratch->touched_count; ++i) {
        size_t index = scratch->touched[i];
        if (index >= scratch->capacity) {
            return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
        }
        scratch->g_score[index] = UINT32_MAX;
        scratch->f_score[index] = UINT32_MAX;
        scratch->parent[index] = -1;
        scratch->open[index] = false;
        scratch->closed[index] = false;
        scratch->touched_flags[index] = false;
        scratch->heap_pos[index] = 0u;
    }
    scratch->touched_count = 0u;

    return GAME_PATH_FIND_RESULT_OK;
}

static GamePathFindResult game_pathfind_touch(GamePathQueryScratch *scratch, size_t index) {
    if (!scratch || index >= scratch->capacity) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }
    if (scratch->touched_flags[index]) {
        return GAME_PATH_FIND_RESULT_OK;
    }
    if (scratch->touched_count >= scratch->touched_capacity) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }

    scratch->touched[scratch->touched_count++] = index;
    scratch->touched_flags[index] = true;
    scratch->g_score[index] = UINT32_MAX;
    scratch->f_score[index] = UINT32_MAX;
    scratch->parent[index] = -1;
    scratch->open[index] = false;
    scratch->closed[index] = false;
    scratch->heap_pos[index] = 0u;
    return GAME_PATH_FIND_RESULT_OK;
}

static GameHexAxial game_pathfind_tile_for_index(const GamePathCostMap *map, size_t index) {
    return (GameHexAxial){
        .q = map->grid.q_min + (int32_t)(index / map->grid.r_count),
        .r = map->grid.r_min + (int32_t)(index % map->grid.r_count),
    };
}

static bool game_pathfind_heap_less(const GamePathCostMap *map, const GamePathQueryScratch *scratch, size_t lhs, size_t rhs) {
    if (scratch->f_score[lhs] != scratch->f_score[rhs]) {
        return scratch->f_score[lhs] < scratch->f_score[rhs];
    }
    if (scratch->g_score[lhs] != scratch->g_score[rhs]) {
        return scratch->g_score[lhs] > scratch->g_score[rhs];
    }

    GameHexAxial left = game_pathfind_tile_for_index(map, lhs);
    GameHexAxial right = game_pathfind_tile_for_index(map, rhs);
    return left.q > right.q || (left.q == right.q && left.r < right.r);
}

static void game_pathfind_heap_swap(GamePathQueryScratch *scratch, size_t *heap_count, size_t left, size_t right) {
    (void)heap_count;
    size_t tmp = scratch->heap[left];
    scratch->heap[left] = scratch->heap[right];
    scratch->heap[right] = tmp;
    scratch->heap_pos[scratch->heap[left]] = left + 1u;
    scratch->heap_pos[scratch->heap[right]] = right + 1u;
}

static void game_pathfind_heap_sift_up(const GamePathCostMap *map, GamePathQueryScratch *scratch, size_t *heap_count,
                                       size_t index) {
    while (index > 0u) {
        size_t parent = (index - 1u) / 2u;
        if (!game_pathfind_heap_less(map, scratch, scratch->heap[index], scratch->heap[parent])) {
            break;
        }
        game_pathfind_heap_swap(scratch, heap_count, index, parent);
        index = parent;
    }
}

static void game_pathfind_heap_sift_down(const GamePathCostMap *map, GamePathQueryScratch *scratch, size_t *heap_count,
                                         size_t index) {
    while (1) {
        size_t left = index * 2u + 1u;
        size_t right = left + 1u;
        size_t best = index;
        if (left < *heap_count && game_pathfind_heap_less(map, scratch, scratch->heap[left], scratch->heap[best])) {
            best = left;
        }
        if (right < *heap_count && game_pathfind_heap_less(map, scratch, scratch->heap[right], scratch->heap[best])) {
            best = right;
        }
        if (best == index) {
            break;
        }
        game_pathfind_heap_swap(scratch, heap_count, index, best);
        index = best;
    }
}

static GamePathFindResult game_pathfind_heap_push_or_fix(const GamePathCostMap *map, GamePathQueryScratch *scratch,
                                                         size_t *heap_count, size_t index) {
    if (!map || !scratch || !heap_count || index >= scratch->capacity) {
        return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
    }
    if (scratch->heap_pos[index] == 0u) {
        if (*heap_count >= scratch->heap_capacity) {
            return GAME_PATH_FIND_RESULT_INVALID_ARGUMENT;
        }
        size_t pos = (*heap_count)++;
        scratch->heap[pos] = index;
        scratch->heap_pos[index] = pos + 1u;
        game_pathfind_heap_sift_up(map, scratch, heap_count, pos);
        return GAME_PATH_FIND_RESULT_OK;
    }

    size_t pos = scratch->heap_pos[index] - 1u;
    game_pathfind_heap_sift_up(map, scratch, heap_count, pos);
    return GAME_PATH_FIND_RESULT_OK;
}

static bool game_pathfind_heap_pop(const GamePathCostMap *map, GamePathQueryScratch *scratch, size_t *heap_count,
                                   size_t *out_index) {
    if (!map || !scratch || !heap_count || !out_index || *heap_count == 0u) {
        return false;
    }

    *out_index = scratch->heap[0];
    scratch->heap_pos[*out_index] = 0u;
    --(*heap_count);
    if (*heap_count > 0u) {
        scratch->heap[0] = scratch->heap[*heap_count];
        scratch->heap_pos[scratch->heap[0]] = 1u;
        game_pathfind_heap_sift_down(map, scratch, heap_count, 0u);
    }
    return true;
}

static size_t game_pathfind_capacity_or_zero(const GamePathCostMap *map) {
    return (map && map->grid.q_count && map->grid.r_count) ? game_pathfind_required_capacity(&map->grid) : 0u;
}

GamePathFindResult game_pathfind_query(
    GameHexAxial start,
    GameHexAxial goal,
    const GamePathCostMap *cost_map,
    GamePathQueryScratch *scratch,
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

    if (scratch->capacity < capacity) {
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

    result = game_pathfind_touch(scratch, start_index);
    if (result != GAME_PATH_FIND_RESULT_OK) {
        return result;
    }
    scratch->g_score[start_index] = 0u;
    scratch->f_score[start_index] = (uint32_t)game_hex_axial_distance(start, goal);
    scratch->open[start_index] = true;

    size_t expansions = 0u;
    size_t heap_count = 0u;
    bool found = false;
    result = game_pathfind_heap_push_or_fix(cost_map, scratch, &heap_count, start_index);
    if (result != GAME_PATH_FIND_RESULT_OK) {
        return result;
    }

    while (heap_count > 0u) {
        size_t best_index = (size_t)-1;
        if (!game_pathfind_heap_pop(cost_map, scratch, &heap_count, &best_index)) {
            break;
        }
        if (!scratch->open[best_index] || scratch->closed[best_index]) {
            continue;
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

        GameHexAxial best = game_pathfind_tile_for_index(cost_map, best_index);

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
            result = game_pathfind_touch(scratch, next_index);
            if (result != GAME_PATH_FIND_RESULT_OK) {
                return result;
            }
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
                result = game_pathfind_heap_push_or_fix(cost_map, scratch, &heap_count, next_index);
                if (result != GAME_PATH_FIND_RESULT_OK) {
                    return result;
                }
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
        out_path[path_length++] = game_pathfind_tile_for_index(cost_map, (size_t)cursor);
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
