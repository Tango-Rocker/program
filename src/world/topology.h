#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "world/hex.h"
#include "world/world_map.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t GameWorldTopologyRegionId;

typedef enum {
    GAME_WORLD_TOPOLOGY_RESULT_OK = 0,
    GAME_WORLD_TOPOLOGY_RESULT_INVALID_ARGUMENT = 1,
    GAME_WORLD_TOPOLOGY_RESULT_OUT_OF_MEMORY = 2,
    GAME_WORLD_TOPOLOGY_RESULT_NOT_FOUND = 3,
} GameWorldTopologyResult;

typedef bool (*GameWorldTopologyPassableFn)(int32_t tile_value);
typedef bool (*GameWorldTopologyPortalFn)(int32_t tile_value);

typedef struct {
    GameHexAxial tile;
    GameWorldTopologyRegionId region_id;
} GameWorldTopologyTileRegion;

typedef struct {
    GameWorldTopologyRegionId region_a;
    GameWorldTopologyRegionId region_b;
    GameHexAxial portal;
} GameWorldTopologyPortal;

typedef struct {
    GameWorldTopologyTileRegion *tile_regions;
    size_t tile_region_count;

    GameWorldTopologyPortal *portals;
    size_t portal_count;
} GameWorldTopology;

void game_world_topology_init(GameWorldTopology *topology);
void game_world_topology_destroy(GameWorldTopology *topology);

GameWorldTopologyResult game_world_topology_rebuild(
    GameWorldTopology *topology,
    const GameWorldMap *map,
    GameWorldTopologyPassableFn is_passable,
    GameWorldTopologyPortalFn is_portal
);

size_t game_world_topology_region_count(const GameWorldTopology *topology);
size_t game_world_topology_portal_count(const GameWorldTopology *topology);

GameWorldTopologyResult game_world_topology_region_of_tile(const GameWorldTopology *topology, GameHexAxial tile,
                                                        GameWorldTopologyRegionId *out_region_id);
bool game_world_topology_same_region(const GameWorldTopology *topology, GameHexAxial first, GameHexAxial second);
GameWorldTopologyResult game_world_topology_portal_at(const GameWorldTopology *topology, size_t index, GameWorldTopologyPortal *out_portal);

#ifdef __cplusplus
}
#endif
