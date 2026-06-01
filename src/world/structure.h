#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "world/hex.h"
#include "world/world_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_STRUCTURE_MAX_FOOTPRINT_TILES 16u

typedef enum {
    GAME_STRUCTURE_RESULT_OK = 0,
    GAME_STRUCTURE_RESULT_INVALID_ARGUMENT = 1,
    GAME_STRUCTURE_RESULT_INVALID_DEFINITION = 2,
    GAME_STRUCTURE_RESULT_NOT_FOUND = 3,
    GAME_STRUCTURE_RESULT_OUT_OF_SPACE = 4,
    GAME_STRUCTURE_RESULT_ALREADY_USED = 5,
    GAME_STRUCTURE_RESULT_NO_SPACE = 6,
    GAME_STRUCTURE_RESULT_INVALID_STATE = 7,
} GameStructureResult;

typedef enum {
    GAME_STRUCTURE_TILE_FLAG_NONE = 0u,
    GAME_STRUCTURE_TILE_FLAG_BLOCKS_PATH = 1u << 0,
    GAME_STRUCTURE_TILE_FLAG_PORTAL = 1u << 1,
    GAME_STRUCTURE_TILE_FLAG_EMITS_LIGHT = 1u << 2,
} GameStructureTileFlags;

typedef enum {
    GAME_STRUCTURE_FIELD_FLAG_NONE = 0u,
    GAME_STRUCTURE_FIELD_FLAG_NOISE = 1u << 0,
    GAME_STRUCTURE_FIELD_FLAG_LIGHT = 1u << 1,
    GAME_STRUCTURE_FIELD_FLAG_SCENT = 1u << 2,
    GAME_STRUCTURE_FIELD_FLAG_BLOOD = 1u << 3,
} GameStructureFieldFlags;

typedef struct {
    int32_t q;
    int32_t r;
} GameStructureFootprintOffset;

typedef struct {
    uint32_t id;
    uint32_t tile_flags;
    uint32_t field_flags;
    size_t footprint_count;
    const GameStructureFootprintOffset *footprint;
} GameStructureDefinition;

typedef struct {
    uint32_t slot;
    uint32_t version;
} GameStructurePlacementHandle;

typedef struct {
    GameStructurePlacementHandle handle;
    uint32_t stable_id;
    bool in_use;
    bool topology_dirty_marked;
    GameHexAxial anchor;
    uint32_t definition_id;
    size_t footprint_count;
    GameHexAxial footprint[GAME_STRUCTURE_MAX_FOOTPRINT_TILES];
    int32_t previous_values[GAME_STRUCTURE_MAX_FOOTPRINT_TILES];
} GameStructurePlacement;

typedef struct {
    const GameStructureDefinition *definitions;
    size_t definition_count;
    GameStructurePlacement *placements;
    size_t placement_capacity;
    uint32_t next_stable_id;
    bool topology_dirty;
} GameStructureSystem;

bool game_structure_is_topology_dirty(const GameStructureSystem *system);
void game_structure_mark_topology_clean(GameStructureSystem *system);

GameStructureResult game_structure_init(
    GameStructureSystem *system,
    const GameStructureDefinition *definitions,
    size_t definition_count,
    GameStructurePlacement *placements,
    size_t placement_capacity,
    uint32_t next_stable_id
);

size_t game_structure_compute_footprint_tiles(
    const GameStructureDefinition *definition,
    GameHexAxial anchor,
    GameHexAxial *out_tiles,
    size_t out_capacity
);

GameStructureResult game_structure_validate_placement(
    const GameStructureSystem *system,
    const GameWorldMap *map,
    uint32_t structure_definition_id,
    GameHexAxial anchor,
    bool allow_missing_chunks
);

GameStructureResult game_structure_place(
    GameStructureSystem *system,
    GameWorldMap *map,
    uint32_t structure_definition_id,
    GameHexAxial anchor,
    GameStructurePlacementHandle *out_handle
);

GameStructureResult game_structure_remove(GameStructureSystem *system, GameWorldMap *map, GameStructurePlacementHandle handle);

GameStructureResult game_structure_status(
    const GameStructureSystem *system,
    GameStructurePlacementHandle handle,
    GameStructurePlacement *out_placement
);

bool game_structure_tile_has_structure(int32_t map_value);
bool game_structure_tile_blocks_path(int32_t map_value);

#ifdef __cplusplus
}
#endif
