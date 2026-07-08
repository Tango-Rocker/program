#include "world/topology.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    GameHexAxial tile;
    int32_t value;
} TopologyTile;

typedef struct {
    TopologyTile *items;
    size_t count;
    size_t capacity;
    GameWorldTopologyResult result;
} TopologyTileArray;

typedef struct {
    GameWorldTopologyPortal *items;
    size_t count;
    size_t capacity;
} TopologyPortalArray;

typedef struct {
    TopologyTileArray passable;
    TopologyTileArray portals;
    TopologyPortalArray portal_edges;
    GameWorldTopologyPassableFn is_passable;
    GameWorldTopologyPortalFn is_portal;
} TopologyCollectContext;

static bool topology_default_is_portal(int32_t tile_value) {
    (void)tile_value;
    return false;
}

static int topology_cmp_hex(GameHexAxial lhs, GameHexAxial rhs) {
    if (lhs.q < rhs.q) {
        return -1;
    }
    if (lhs.q > rhs.q) {
        return 1;
    }
    if (lhs.r < rhs.r) {
        return -1;
    }
    if (lhs.r > rhs.r) {
        return 1;
    }
    return 0;
}

static bool topology_hex_equal(GameHexAxial lhs, GameHexAxial rhs) {
    return topology_cmp_hex(lhs, rhs) == 0;
}

static int topology_region_id_cmp(GameWorldTopologyRegionId lhs, GameWorldTopologyRegionId rhs) {
    if (lhs < rhs) {
        return -1;
    }
    if (lhs > rhs) {
        return 1;
    }
    return 0;
}

static int topology_cmp_portal(const void *lhs, const void *rhs) {
    const GameWorldTopologyPortal *left = (const GameWorldTopologyPortal *)lhs;
    const GameWorldTopologyPortal *right = (const GameWorldTopologyPortal *)rhs;

    int compare = topology_region_id_cmp(left->region_a, right->region_a);
    if (compare != 0) {
        return compare;
    }

    compare = topology_region_id_cmp(left->region_b, right->region_b);
    if (compare != 0) {
        return compare;
    }

    return topology_cmp_hex(left->portal, right->portal);
}

static int topology_cmp_tile_item(const void *lhs, const void *rhs) {
    const TopologyTile *left = (const TopologyTile *)lhs;
    const TopologyTile *right = (const TopologyTile *)rhs;
    return topology_cmp_hex(left->tile, right->tile);
}

static int topology_cmp_tile_region(const void *lhs, const void *rhs) {
    const GameWorldTopologyTileRegion *left = (const GameWorldTopologyTileRegion *)lhs;
    const GameWorldTopologyTileRegion *right = (const GameWorldTopologyTileRegion *)rhs;
    return topology_cmp_hex(left->tile, right->tile);
}

static void topology_free_tile_array(TopologyTileArray *array) {
    free(array->items);
    array->items = NULL;
    array->count = 0u;
    array->capacity = 0u;
    array->result = GAME_WORLD_TOPOLOGY_RESULT_OK;
}

static void topology_free_portal_array(TopologyPortalArray *array) {
    free(array->items);
    array->items = NULL;
    array->count = 0u;
    array->capacity = 0u;
}

static GameWorldTopologyResult topology_push_tile(TopologyTileArray *array, GameHexAxial tile, int32_t value) {
    if (!array || array->result != GAME_WORLD_TOPOLOGY_RESULT_OK) {
        return GAME_WORLD_TOPOLOGY_RESULT_INVALID_ARGUMENT;
    }

    if (array->count == array->capacity) {
        size_t target_capacity = array->capacity == 0u ? 16u : (array->capacity * 2u);
        TopologyTile *next = (TopologyTile *)realloc(array->items, target_capacity * sizeof(TopologyTile));
        if (!next) {
            array->result = GAME_WORLD_TOPOLOGY_RESULT_OUT_OF_MEMORY;
            return array->result;
        }
        array->items = next;
        array->capacity = target_capacity;
    }

    array->items[array->count++] = (TopologyTile){.tile = tile, .value = value};
    return GAME_WORLD_TOPOLOGY_RESULT_OK;
}

static GameWorldTopologyResult topology_push_portal_edge(TopologyPortalArray *array, GameWorldTopologyPortal edge) {
    if (!array) {
        return GAME_WORLD_TOPOLOGY_RESULT_INVALID_ARGUMENT;
    }

    if (array->count == array->capacity) {
        size_t target_capacity = array->capacity == 0u ? 8u : (array->capacity * 2u);
        GameWorldTopologyPortal *next = (GameWorldTopologyPortal *)realloc(
            array->items,
            target_capacity * sizeof(GameWorldTopologyPortal)
        );
        if (!next) {
            return GAME_WORLD_TOPOLOGY_RESULT_OUT_OF_MEMORY;
        }
        array->items = next;
        array->capacity = target_capacity;
    }

    array->items[array->count++] = edge;
    return GAME_WORLD_TOPOLOGY_RESULT_OK;
}

static int topology_find_passable_index(const TopologyTile *tiles, size_t count, GameHexAxial tile) {
    size_t lo = 0u;
    size_t hi = count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        int cmp = topology_cmp_hex(tiles[mid].tile, tile);
        if (cmp == 0) {
            return (int)mid;
        }
        if (cmp < 0) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    return -1;
}

static void topology_collect_visitor(void *user, GameWorldMapTileVisit visit) {
    TopologyCollectContext *ctx = (TopologyCollectContext *)user;
    if (!ctx || ctx->passable.result != GAME_WORLD_TOPOLOGY_RESULT_OK) {
        return;
    }

    if (ctx->is_passable(visit.value)) {
        topology_push_tile(&ctx->passable, visit.tile, visit.value);
    }
    if (ctx->is_portal(visit.value)) {
        topology_push_tile(&ctx->portals, visit.tile, visit.value);
    }
}

static void topology_deduplicate_portals(TopologyPortalArray *portals) {
    if (!portals || portals->count <= 1u) {
        return;
    }

    qsort(portals->items, portals->count, sizeof(GameWorldTopologyPortal), topology_cmp_portal);
    size_t write = 1u;
    for (size_t read = 1u; read < portals->count; ++read) {
        if (topology_cmp_portal(&portals->items[write - 1u], &portals->items[read]) != 0) {
            portals->items[write++] = portals->items[read];
        }
    }
    portals->count = write;
}

void game_world_topology_init(GameWorldTopology *topology) {
    if (!topology) {
        return;
    }
    *topology = (GameWorldTopology){0};
}

void game_world_topology_destroy(GameWorldTopology *topology) {
    if (!topology) {
        return;
    }

    free(topology->tile_regions);
    free(topology->portals);
    *topology = (GameWorldTopology){0};
}

size_t game_world_topology_region_count(const GameWorldTopology *topology) {
    if (!topology) {
        return 0u;
    }

    size_t max_region_id = 0u;
    for (size_t i = 0u; i < topology->tile_region_count; ++i) {
        if ((size_t)topology->tile_regions[i].region_id > max_region_id) {
            max_region_id = topology->tile_regions[i].region_id;
        }
    }
    return max_region_id;
}

size_t game_world_topology_portal_count(const GameWorldTopology *topology) {
    return topology ? topology->portal_count : 0u;
}

GameWorldTopologyResult game_world_topology_region_of_tile(const GameWorldTopology *topology, GameHexAxial tile,
                                                        GameWorldTopologyRegionId *out_region_id) {
    if (!topology || !out_region_id) {
        return GAME_WORLD_TOPOLOGY_RESULT_INVALID_ARGUMENT;
    }

    size_t lo = 0u;
    size_t hi = topology->tile_region_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        int cmp = topology_cmp_hex(topology->tile_regions[mid].tile, tile);
        if (cmp == 0) {
            *out_region_id = topology->tile_regions[mid].region_id;
            return GAME_WORLD_TOPOLOGY_RESULT_OK;
        }
        if (cmp < 0) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }

    return GAME_WORLD_TOPOLOGY_RESULT_NOT_FOUND;
}

bool game_world_topology_same_region(const GameWorldTopology *topology, GameHexAxial first, GameHexAxial second) {
    GameWorldTopologyRegionId first_region = 0u;
    GameWorldTopologyRegionId second_region = 0u;

    if (game_world_topology_region_of_tile(topology, first, &first_region) != GAME_WORLD_TOPOLOGY_RESULT_OK) {
        return false;
    }

    if (game_world_topology_region_of_tile(topology, second, &second_region) != GAME_WORLD_TOPOLOGY_RESULT_OK) {
        return false;
    }

    return first_region == second_region;
}

GameWorldTopologyResult game_world_topology_portal_at(
    const GameWorldTopology *topology,
    size_t index,
    GameWorldTopologyPortal *out_portal
) {
    if (!topology || !out_portal) {
        return GAME_WORLD_TOPOLOGY_RESULT_INVALID_ARGUMENT;
    }
    if (index >= topology->portal_count) {
        return GAME_WORLD_TOPOLOGY_RESULT_NOT_FOUND;
    }

    *out_portal = topology->portals[index];
    return GAME_WORLD_TOPOLOGY_RESULT_OK;
}

GameWorldTopologyResult game_world_topology_rebuild(
    GameWorldTopology *topology,
    const GameWorldMap *map,
    GameWorldTopologyPassableFn is_passable,
    GameWorldTopologyPortalFn is_portal
) {
    if (!topology || !map || !is_passable) {
        return GAME_WORLD_TOPOLOGY_RESULT_INVALID_ARGUMENT;
    }

    if (!is_portal) {
        is_portal = topology_default_is_portal;
    }

    TopologyCollectContext context = {
        .passable = {0},
        .portals = {0},
        .portal_edges = {0},
        .is_passable = is_passable,
        .is_portal = is_portal,
    };

    GameWorldMapResult map_result = game_world_map_for_each_tile(map, topology_collect_visitor, &context);
    if (map_result != GAME_WORLD_MAP_RESULT_OK && map_result != GAME_WORLD_MAP_RESULT_CHUNK_MISSING) {
        return GAME_WORLD_TOPOLOGY_RESULT_INVALID_ARGUMENT;
    }
    if (context.passable.result != GAME_WORLD_TOPOLOGY_RESULT_OK) {
        topology_free_tile_array(&context.passable);
        topology_free_tile_array(&context.portals);
        return context.passable.result;
    }
    if (context.portals.result != GAME_WORLD_TOPOLOGY_RESULT_OK) {
        topology_free_tile_array(&context.passable);
        topology_free_tile_array(&context.portals);
        return context.portals.result;
    }

    game_world_topology_destroy(topology);
    if (context.passable.count == 0u) {
        topology_free_tile_array(&context.passable);
        topology_free_tile_array(&context.portals);
        return GAME_WORLD_TOPOLOGY_RESULT_OK;
    }

    qsort(context.passable.items, context.passable.count, sizeof(TopologyTile), topology_cmp_tile_item);
    if (context.portals.count > 1u) {
        qsort(context.portals.items, context.portals.count, sizeof(TopologyTile), topology_cmp_tile_item);
    }

    GameWorldTopologyRegionId *tile_region_ids = (GameWorldTopologyRegionId *)calloc(
        context.passable.count,
        sizeof(GameWorldTopologyRegionId)
    );
    if (!tile_region_ids) {
        topology_free_tile_array(&context.passable);
        topology_free_tile_array(&context.portals);
        return GAME_WORLD_TOPOLOGY_RESULT_OUT_OF_MEMORY;
    }

    size_t *queue = (size_t *)malloc(context.passable.count * sizeof(size_t));
    if (!queue) {
        free(tile_region_ids);
        topology_free_tile_array(&context.passable);
        topology_free_tile_array(&context.portals);
        return GAME_WORLD_TOPOLOGY_RESULT_OUT_OF_MEMORY;
    }

    GameWorldTopologyTileRegion *tile_regions = (GameWorldTopologyTileRegion *)malloc(
        context.passable.count * sizeof(GameWorldTopologyTileRegion)
    );
    if (!tile_regions) {
        free(queue);
        free(tile_region_ids);
        topology_free_tile_array(&context.passable);
        topology_free_tile_array(&context.portals);
        return GAME_WORLD_TOPOLOGY_RESULT_OUT_OF_MEMORY;
    }

    GameWorldTopologyRegionId next_region_id = 0u;
    for (size_t i = 0u; i < context.passable.count; ++i) {
        if (tile_region_ids[i] != 0u) {
            continue;
        }
        tile_region_ids[i] = ++next_region_id;

        size_t queue_read = 0u;
        size_t queue_write = 0u;
        queue[queue_write++] = i;

        while (queue_read < queue_write) {
            size_t current_index = queue[queue_read++];
            GameHexAxial neighbors[6] = {0};
            game_hex_axial_neighbors(context.passable.items[current_index].tile, neighbors);

            for (size_t n = 0u; n < 6u; ++n) {
                int32_t neighbor_index = topology_find_passable_index(
                    context.passable.items,
                    context.passable.count,
                    neighbors[n]
                );
                if (neighbor_index < 0) {
                    continue;
                }

                size_t nindex = (size_t)neighbor_index;
                if (tile_region_ids[nindex] == 0u) {
                    tile_region_ids[nindex] = next_region_id;
                    queue[queue_write++] = nindex;
                }
            }
        }
    }

    for (size_t i = 0u; i < context.passable.count; ++i) {
        tile_regions[i] = (GameWorldTopologyTileRegion){
            .tile = context.passable.items[i].tile,
            .region_id = tile_region_ids[i],
        };
    }
    free(tile_region_ids);
    free(queue);

    for (size_t i = 0u; i < context.portals.count; ++i) {
        GameHexAxial neighbors[6] = {0};
        game_hex_axial_neighbors(context.portals.items[i].tile, neighbors);

        GameWorldTopologyRegionId local_regions[6] = {0};
        size_t local_region_count = 0u;
        for (size_t n = 0u; n < 6u; ++n) {
            int32_t neighbor_index = topology_find_passable_index(
                context.passable.items,
                context.passable.count,
                neighbors[n]
            );
            if (neighbor_index < 0) {
                continue;
            }

            GameWorldTopologyRegionId candidate = tile_regions[(size_t)neighbor_index].region_id;
            if (candidate == 0u) {
                continue;
            }

            bool already = false;
            for (size_t j = 0u; j < local_region_count; ++j) {
                if (local_regions[j] == candidate) {
                    already = true;
                    break;
                }
            }
            if (!already) {
                local_regions[local_region_count++] = candidate;
            }
        }

        for (size_t left = 0u; left < local_region_count; ++left) {
            for (size_t right = left + 1u; right < local_region_count; ++right) {
                GameWorldTopologyRegionId from = local_regions[left];
                GameWorldTopologyRegionId to = local_regions[right];
                if (from > to) {
                    GameWorldTopologyRegionId tmp = from;
                    from = to;
                    to = tmp;
                }
                GameWorldTopologyResult edge_result = topology_push_portal_edge(
                    &context.portal_edges,
                    (GameWorldTopologyPortal){
                        .region_a = from,
                        .region_b = to,
                        .portal = context.portals.items[i].tile,
                    }
                );
                if (edge_result != GAME_WORLD_TOPOLOGY_RESULT_OK) {
                    free(tile_regions);
                    topology_free_tile_array(&context.passable);
                    topology_free_tile_array(&context.portals);
                    topology_free_portal_array(&context.portal_edges);
                    return edge_result;
                }
            }
        }
    }

    topology_deduplicate_portals(&context.portal_edges);
    qsort(tile_regions, context.passable.count, sizeof(GameWorldTopologyTileRegion), topology_cmp_tile_region);

    topology->tile_regions = tile_regions;
    topology->tile_region_count = context.passable.count;
    topology->portals = context.portal_edges.items;
    topology->portal_count = context.portal_edges.count;

    topology_free_tile_array(&context.passable);
    topology_free_tile_array(&context.portals);
    context.portal_edges.items = NULL;
    context.portal_edges.count = 0u;
    context.portal_edges.capacity = 0u;
    return GAME_WORLD_TOPOLOGY_RESULT_OK;
}
