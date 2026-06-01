#include "world/structure.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

static const int32_t GAME_STRUCTURE_VALUE_MAGIC = 0x40000000;
static const int32_t GAME_STRUCTURE_VALUE_MAGIC_MASK = (int32_t)0xC0000000;
static const int32_t GAME_STRUCTURE_VALUE_PORTAL_FLAG = 0x20000000;
static const int32_t GAME_STRUCTURE_VALUE_BLOCKS_PATH_FLAG = 0x10000000;
static const int32_t GAME_STRUCTURE_VALUE_FIELD_MASK = 0x0F000000;
static const int32_t GAME_STRUCTURE_VALUE_TYPE_MASK = 0x0000FFFF;
static int structure_cmp_tiles(const void *lhs, const void *rhs) {
    const GameHexAxial *left = (const GameHexAxial *)lhs;
    const GameHexAxial *right = (const GameHexAxial *)rhs;

    if (left->q < right->q) {
        return -1;
    }
    if (left->q > right->q) {
        return 1;
    }
    if (left->r < right->r) {
        return -1;
    }
    if (left->r > right->r) {
        return 1;
    }
    return 0;
}

static const GameStructureDefinition *game_structure_get_definition(
    const GameStructureSystem *system,
    uint32_t definition_id
) {
    if (!system || !system->definitions) {
        return NULL;
    }

    for (size_t i = 0u; i < system->definition_count; ++i) {
        if (system->definitions[i].id == definition_id) {
            return &system->definitions[i];
        }
    }
    return NULL;
}

static bool game_structure_handle_matches(
    const GameStructurePlacement *placement,
    GameStructurePlacementHandle handle
) {
    return placement->in_use && placement->handle.slot == handle.slot && placement->handle.version == handle.version;
}

static int32_t game_structure_make_tile_value(
    const GameStructureDefinition *definition,
    uint32_t stable_id
) {
    int32_t value = GAME_STRUCTURE_VALUE_MAGIC;
    if ((definition->tile_flags & GAME_STRUCTURE_TILE_FLAG_BLOCKS_PATH) != 0u) {
        value |= GAME_STRUCTURE_VALUE_BLOCKS_PATH_FLAG;
    }
    if ((definition->tile_flags & GAME_STRUCTURE_TILE_FLAG_PORTAL) != 0u) {
        value |= GAME_STRUCTURE_VALUE_PORTAL_FLAG;
    }
    value |= (int32_t)((definition->field_flags << 24u) & GAME_STRUCTURE_VALUE_FIELD_MASK);
    value |= (int32_t)(stable_id & GAME_STRUCTURE_VALUE_TYPE_MASK);
    return value;
}

static bool game_structure_value_is_compatible(int32_t value) {
    return (value & GAME_STRUCTURE_VALUE_MAGIC_MASK) == GAME_STRUCTURE_VALUE_MAGIC;
}

static uint32_t game_structure_next_version(uint32_t version) {
    return version == UINT32_MAX ? 1u : (version + 1u);
}

static void game_structure_init_slot(GameStructurePlacement *slot) {
    if (!slot) {
        return;
    }

    slot->in_use = false;
    slot->topology_dirty_marked = false;
    slot->handle.version = slot->handle.version == 0u ? 1u : slot->handle.version;
    slot->stable_id = 0u;
    slot->definition_id = 0u;
    slot->footprint_count = 0u;
    slot->anchor = (GameHexAxial){0, 0};
    for (size_t i = 0u; i < GAME_STRUCTURE_MAX_FOOTPRINT_TILES; ++i) {
        slot->footprint[i] = (GameHexAxial){0, 0};
    slot->previous_values[i] = 0;
    }
}

static uint32_t game_structure_next_slot_version(uint32_t version) {
    return version == UINT32_MAX ? 1u : (version + 1u);
}

size_t game_structure_compute_footprint_tiles(
    const GameStructureDefinition *definition,
    GameHexAxial anchor,
    GameHexAxial *out_tiles,
    size_t out_capacity
) {
    if (!definition || !out_tiles || out_capacity == 0u || definition->footprint == NULL || definition->footprint_count == 0u) {
        return 0u;
    }
    if (definition->footprint_count > out_capacity) {
        return 0u;
    }
    if (definition->footprint_count > GAME_STRUCTURE_MAX_FOOTPRINT_TILES) {
        return 0u;
    }

    size_t emit = 0u;
    for (size_t i = 0u; i < definition->footprint_count; ++i) {
        out_tiles[emit++] = (GameHexAxial){
            .q = anchor.q + definition->footprint[i].q,
            .r = anchor.r + definition->footprint[i].r,
        };
    }

    if (emit > 1u) {
        qsort(out_tiles, emit, sizeof(GameHexAxial), structure_cmp_tiles);
    }
    return emit;
}

bool game_structure_is_topology_dirty(const GameStructureSystem *system) {
    return system && system->topology_dirty;
}

void game_structure_mark_topology_clean(GameStructureSystem *system) {
    if (system) {
        system->topology_dirty = false;
    }
}

GameStructureResult game_structure_init(
    GameStructureSystem *system,
    const GameStructureDefinition *definitions,
    size_t definition_count,
    GameStructurePlacement *placements,
    size_t placement_capacity,
    uint32_t next_stable_id
) {
    if (!system || !placements || !definitions || definition_count == 0u || placement_capacity == 0u || next_stable_id == 0u) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    system->definitions = definitions;
    system->definition_count = definition_count;
    system->placements = placements;
    system->placement_capacity = placement_capacity;
    system->next_stable_id = next_stable_id;
    system->topology_dirty = false;

    for (size_t i = 0u; i < placement_capacity; ++i) {
        placements[i] = (GameStructurePlacement){0};
        placements[i].handle.slot = (uint32_t)i;
        placements[i].handle.version = 1u;
        game_structure_init_slot(&placements[i]);
    }

    return GAME_STRUCTURE_RESULT_OK;
}

GameStructureResult game_structure_validate_placement(
    const GameStructureSystem *system,
    const GameWorldMap *map,
    uint32_t structure_definition_id,
    GameHexAxial anchor,
    bool allow_missing_chunks
) {
    if (!system || !map || !system->definitions) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    const GameStructureDefinition *definition = game_structure_get_definition(system, structure_definition_id);
    if (!definition) {
        return GAME_STRUCTURE_RESULT_INVALID_DEFINITION;
    }

    if (definition->footprint == NULL || definition->footprint_count == 0u) {
        return GAME_STRUCTURE_RESULT_INVALID_DEFINITION;
    }

    if (definition->footprint_count > GAME_STRUCTURE_MAX_FOOTPRINT_TILES) {
        return GAME_STRUCTURE_RESULT_OUT_OF_SPACE;
    }

    GameHexAxial footprint_tiles[GAME_STRUCTURE_MAX_FOOTPRINT_TILES] = {0};
    size_t footprint_count = game_structure_compute_footprint_tiles(
        definition,
        anchor,
        footprint_tiles,
        GAME_STRUCTURE_MAX_FOOTPRINT_TILES
    );
    if (footprint_count != definition->footprint_count) {
        return GAME_STRUCTURE_RESULT_OUT_OF_SPACE;
    }

    for (size_t i = 0u; i < footprint_count; ++i) {
        int32_t existing = 0;
        GameWorldMapResult result = game_world_map_get(map, footprint_tiles[i], &existing);
        if (result == GAME_WORLD_MAP_RESULT_CHUNK_MISSING) {
            if (allow_missing_chunks) {
                return GAME_STRUCTURE_RESULT_OK;
            }
            return GAME_STRUCTURE_RESULT_NO_SPACE;
        }
        if (result != GAME_WORLD_MAP_RESULT_OK) {
            return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
        }
        if (game_structure_value_is_compatible(existing)) {
            return GAME_STRUCTURE_RESULT_ALREADY_USED;
        }
    }

    return GAME_STRUCTURE_RESULT_OK;
}

GameStructureResult game_structure_place(
    GameStructureSystem *system,
    GameWorldMap *map,
    uint32_t structure_definition_id,
    GameHexAxial anchor,
    GameStructurePlacementHandle *out_handle
) {
    if (!system || !map || !out_handle || !system->placements) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    const GameStructureDefinition *definition = game_structure_get_definition(system, structure_definition_id);
    if (!definition) {
        return GAME_STRUCTURE_RESULT_INVALID_DEFINITION;
    }

    GameStructureResult validate = game_structure_validate_placement(system, map, structure_definition_id, anchor, false);
    if (validate != GAME_STRUCTURE_RESULT_OK) {
        return validate;
    }

    size_t slot_index = system->placement_capacity;
    for (size_t i = 0u; i < system->placement_capacity; ++i) {
        if (!system->placements[i].in_use) {
            slot_index = i;
            break;
        }
    }
    if (slot_index == system->placement_capacity) {
        return GAME_STRUCTURE_RESULT_OUT_OF_SPACE;
    }

    GameHexAxial footprint_tiles[GAME_STRUCTURE_MAX_FOOTPRINT_TILES] = {0};
    size_t footprint_count = game_structure_compute_footprint_tiles(
        definition,
        anchor,
        footprint_tiles,
        GAME_STRUCTURE_MAX_FOOTPRINT_TILES
    );
    if (footprint_count == 0u || footprint_count != definition->footprint_count) {
        return GAME_STRUCTURE_RESULT_OUT_OF_SPACE;
    }

    int32_t previous_values[GAME_STRUCTURE_MAX_FOOTPRINT_TILES] = {0};
    GameStructurePlacement candidate = {
        .handle = {
            .slot = (uint32_t)slot_index,
            .version = system->placements[slot_index].handle.version,
        },
        .stable_id = system->next_stable_id,
        .in_use = true,
        .topology_dirty_marked = true,
        .anchor = anchor,
        .definition_id = definition->id,
        .footprint_count = footprint_count,
    };

    for (size_t i = 0u; i < footprint_count; ++i) {
        int32_t existing = 0;
        if (game_world_map_get(map, footprint_tiles[i], &existing) != GAME_WORLD_MAP_RESULT_OK) {
            return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
        }
        previous_values[i] = existing;
    }

    for (size_t i = 0u; i < footprint_count; ++i) {
        GameWorldMapResult set_result = game_world_map_set(map, footprint_tiles[i], game_structure_make_tile_value(definition, candidate.stable_id));
        if (set_result != GAME_WORLD_MAP_RESULT_OK) {
            for (size_t rollback = 0u; rollback < i; ++rollback) {
                (void)game_world_map_set(map, footprint_tiles[rollback], previous_values[rollback]);
            }
            return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
        }

        candidate.footprint[i] = footprint_tiles[i];
        candidate.previous_values[i] = previous_values[i];
    }

    if (system->next_stable_id == UINT32_MAX) {
        system->next_stable_id = 1u;
    } else {
        ++system->next_stable_id;
    }

    if (system->next_stable_id == 0u) {
        system->next_stable_id = 1u;
    }

    system->placements[slot_index] = candidate;
    *out_handle = system->placements[slot_index].handle;
    system->topology_dirty = true;
    return GAME_STRUCTURE_RESULT_OK;
}

GameStructureResult game_structure_remove(
    GameStructureSystem *system,
    GameWorldMap *map,
    GameStructurePlacementHandle handle
) {
    if (!system || !map || !system->placements) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    if (handle.slot >= system->placement_capacity) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    GameStructurePlacement *slot = &system->placements[handle.slot];
    if (!game_structure_handle_matches(slot, handle)) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < slot->footprint_count; ++i) {
        if (game_world_map_set(map, slot->footprint[i], slot->previous_values[i]) != GAME_WORLD_MAP_RESULT_OK) {
            return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
        }
    }

    slot->topology_dirty_marked = true;
    slot->handle.version = game_structure_next_slot_version(slot->handle.version);
    game_structure_init_slot(slot);
    system->topology_dirty = true;
    return GAME_STRUCTURE_RESULT_OK;
}

GameStructureResult game_structure_status(
    const GameStructureSystem *system,
    GameStructurePlacementHandle handle,
    GameStructurePlacement *out_placement
) {
    if (!system || !out_placement || !system->placements) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    if (handle.slot >= system->placement_capacity) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    const GameStructurePlacement *slot = &system->placements[handle.slot];
    if (!game_structure_handle_matches(slot, handle)) {
        return GAME_STRUCTURE_RESULT_INVALID_ARGUMENT;
    }

    *out_placement = *slot;
    return GAME_STRUCTURE_RESULT_OK;
}

bool game_structure_tile_has_structure(int32_t map_value) {
    return game_structure_value_is_compatible(map_value);
}

bool game_structure_tile_blocks_path(int32_t map_value) {
    return game_structure_value_is_compatible(map_value) && (map_value & GAME_STRUCTURE_VALUE_BLOCKS_PATH_FLAG) != 0;
}
