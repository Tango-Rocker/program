#include "world/world_map.h"

#include <stdlib.h>
#include <string.h>

static int world_map_cmp_chunk_key(GameHexChunkKey a, GameHexChunkKey b) {
    if (a.cx < b.cx) {
        return -1;
    }
    if (a.cx > b.cx) {
        return 1;
    }
    if (a.cy < b.cy) {
        return -1;
    }
    if (a.cy > b.cy) {
        return 1;
    }
    if (a.cz < b.cz) {
        return -1;
    }
    if (a.cz > b.cz) {
        return 1;
    }
    return 0;
}

static bool world_map_local_valid(GameHexChunkLocal local, int32_t chunk_radius) {
    return local.lx >= 0 && local.ly >= 0 && local.lz >= 0 && local.lx < chunk_radius && local.ly < chunk_radius
           && local.lz < chunk_radius;
}

static uint32_t world_map_local_to_linear(GameHexChunkLocal local, int32_t chunk_radius) {
    return (uint32_t)((uint32_t)local.lx * (uint32_t)chunk_radius * (uint32_t)chunk_radius
                      + (uint32_t)local.ly * (uint32_t)chunk_radius
                      + (uint32_t)local.lz);
}

static size_t world_map_chunk_volume(int32_t chunk_radius) {
    return (size_t)chunk_radius * (size_t)chunk_radius * (size_t)chunk_radius;
}

static GameWorldMapResult world_map_validate_initialized(const GameWorldMap *map) {
    if (!map || map->chunk_radius <= 0 || map->chunk_capacity < map->chunk_count) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }
    return GAME_WORLD_MAP_RESULT_OK;
}

static GameWorldMapChunk *world_map_find_chunk(const GameWorldMap *map, GameHexChunkKey key) {
    if (!map || map->chunk_count == 0) {
        return NULL;
    }

    size_t lo = 0u;
    size_t hi = map->chunk_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        int cmp = world_map_cmp_chunk_key(map->chunks[mid].key, key);
        if (cmp == 0) {
            return &map->chunks[mid];
        }
        if (cmp < 0) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    return NULL;
}

static GameWorldMapResult world_map_find_chunk_insert_position(const GameWorldMap *map, GameHexChunkKey key, size_t *out_insert_index) {
    if (!map || !out_insert_index) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    size_t lo = 0u;
    size_t hi = map->chunk_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2u;
        int cmp = world_map_cmp_chunk_key(map->chunks[mid].key, key);
        if (cmp == 0) {
            return GAME_WORLD_MAP_RESULT_CHUNK_EXISTS;
        }
        if (cmp < 0) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }

    *out_insert_index = lo;
    return GAME_WORLD_MAP_RESULT_OK;
}

static void world_map_decompose_local(uint32_t linear_index, int32_t chunk_radius, GameHexChunkLocal *local) {
    uint32_t plane = (uint32_t)(chunk_radius * chunk_radius);
    local->lx = (int32_t)(linear_index / plane);
    local->ly = (int32_t)((linear_index / (uint32_t)chunk_radius) % (uint32_t)chunk_radius);
    local->lz = (int32_t)(linear_index % (uint32_t)chunk_radius);
}

static GameWorldMapResult world_map_cube_to_local_index(GameHexCube cube, int32_t chunk_radius, uint32_t *out_index) {
    if (chunk_radius <= 0 || !out_index) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    GameHexChunkLocal local = game_hex_cube_to_chunk_local(cube, chunk_radius);
    if (!world_map_local_valid(local, chunk_radius)) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    *out_index = world_map_local_to_linear(local, chunk_radius);
    return GAME_WORLD_MAP_RESULT_OK;
}

GameWorldMapResult game_world_map_init(GameWorldMap *map, int32_t chunk_radius, int32_t default_tile_value) {
    if (!map || chunk_radius <= 0) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    map->chunk_radius = chunk_radius;
    map->default_tile_value = default_tile_value;
    map->chunks = NULL;
    map->chunk_count = 0u;
    map->chunk_capacity = 0u;
    return GAME_WORLD_MAP_RESULT_OK;
}

void game_world_map_destroy(GameWorldMap *map) {
    if (!map) {
        return;
    }

    if (map->chunks) {
        for (size_t i = 0; i < map->chunk_count; ++i) {
            free(map->chunks[i].tile_values);
            map->chunks[i].tile_values = NULL;
        }
        free(map->chunks);
    }

    map->chunks = NULL;
    map->chunk_count = 0u;
    map->chunk_capacity = 0u;
}

size_t game_world_map_chunk_count(const GameWorldMap *map) {
    return map ? map->chunk_count : 0u;
}

GameWorldMapResult game_world_map_create_chunk(GameWorldMap *map, GameHexChunkKey chunk_key) {
    if (world_map_validate_initialized(map) != GAME_WORLD_MAP_RESULT_OK) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    size_t insert_index = 0u;
    GameWorldMapResult insert_status = world_map_find_chunk_insert_position(map, chunk_key, &insert_index);
    if (insert_status != GAME_WORLD_MAP_RESULT_OK) {
        return insert_status;
    }

    size_t chunk_data_size = world_map_chunk_volume(map->chunk_radius);
    int32_t *tile_values = (int32_t *)malloc(sizeof(int32_t) * chunk_data_size);
    if (!tile_values) {
        return GAME_WORLD_MAP_RESULT_OUT_OF_MEMORY;
    }

    for (size_t i = 0; i < chunk_data_size; ++i) {
        tile_values[i] = map->default_tile_value;
    }

    if (map->chunk_count == map->chunk_capacity) {
        size_t target_capacity = map->chunk_capacity == 0u ? 4u : (map->chunk_capacity * 2u);
        GameWorldMapChunk *chunks = (GameWorldMapChunk *)realloc(map->chunks, sizeof(GameWorldMapChunk) * target_capacity);
        if (!chunks) {
            free(tile_values);
            return GAME_WORLD_MAP_RESULT_OUT_OF_MEMORY;
        }
        map->chunks = chunks;
        map->chunk_capacity = target_capacity;
    }

    if (insert_index < map->chunk_count) {
        memmove(&map->chunks[insert_index + 1], &map->chunks[insert_index],
                (map->chunk_count - insert_index) * sizeof(GameWorldMapChunk));
    }

    map->chunks[insert_index] = (GameWorldMapChunk){.key = chunk_key, .tile_values = tile_values};
    ++map->chunk_count;
    return GAME_WORLD_MAP_RESULT_OK;
}

GameWorldMapResult game_world_map_get(const GameWorldMap *map, GameHexAxial position, int32_t *out_value) {
    if (!out_value || !map || map->chunk_radius <= 0) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }
    if (!map->chunks && map->chunk_count != 0u) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    GameHexCube cube = game_hex_axial_to_cube(position);
    GameHexChunkKey chunk_key = game_hex_cube_to_chunk_key(cube, map->chunk_radius);
    GameWorldMapChunk *chunk = world_map_find_chunk(map, chunk_key);
    if (!chunk) {
        return GAME_WORLD_MAP_RESULT_CHUNK_MISSING;
    }

    uint32_t local_index = 0u;
    if (world_map_cube_to_local_index(cube, map->chunk_radius, &local_index) != GAME_WORLD_MAP_RESULT_OK) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }
    *out_value = chunk->tile_values[local_index];
    return GAME_WORLD_MAP_RESULT_OK;
}

GameWorldMapResult game_world_map_set(GameWorldMap *map, GameHexAxial position, int32_t value) {
    if (!map || map->chunk_radius <= 0) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }
    if (!map->chunks && map->chunk_count != 0u) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    GameHexCube cube = game_hex_axial_to_cube(position);
    GameHexChunkKey chunk_key = game_hex_cube_to_chunk_key(cube, map->chunk_radius);
    GameWorldMapChunk *chunk = world_map_find_chunk(map, chunk_key);
    if (!chunk) {
        return GAME_WORLD_MAP_RESULT_CHUNK_MISSING;
    }

    uint32_t local_index = 0u;
    if (world_map_cube_to_local_index(cube, map->chunk_radius, &local_index) != GAME_WORLD_MAP_RESULT_OK) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }
    chunk->tile_values[local_index] = value;
    return GAME_WORLD_MAP_RESULT_OK;
}

GameWorldMapResult game_world_map_get_local(const GameWorldMap *map, GameHexChunkKey chunk_key, GameHexChunkLocal local, int32_t *out_value) {
    if (!map || !out_value || map->chunk_radius <= 0) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }
    if (!world_map_local_valid(local, map->chunk_radius)) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    GameWorldMapChunk *chunk = world_map_find_chunk(map, chunk_key);
    if (!chunk) {
        return GAME_WORLD_MAP_RESULT_CHUNK_MISSING;
    }

    uint32_t local_index = world_map_local_to_linear(local, map->chunk_radius);
    *out_value = chunk->tile_values[local_index];
    return GAME_WORLD_MAP_RESULT_OK;
}

GameWorldMapResult game_world_map_set_local(GameWorldMap *map, GameHexChunkKey chunk_key, GameHexChunkLocal local, int32_t value) {
    if (!map || map->chunk_radius <= 0) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }
    if (!world_map_local_valid(local, map->chunk_radius)) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    GameWorldMapChunk *chunk = world_map_find_chunk(map, chunk_key);
    if (!chunk) {
        return GAME_WORLD_MAP_RESULT_CHUNK_MISSING;
    }

    uint32_t local_index = world_map_local_to_linear(local, map->chunk_radius);
    chunk->tile_values[local_index] = value;
    return GAME_WORLD_MAP_RESULT_OK;
}

GameWorldMapResult game_world_map_for_each_tile(const GameWorldMap *map, GameWorldMapTileVisitor visitor, void *user) {
    if (!map || !visitor || map->chunk_radius <= 0) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    if (!map->chunks && map->chunk_count != 0u) {
        return GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT;
    }

    size_t chunk_count = map->chunk_count;
    size_t tile_count_per_chunk = world_map_chunk_volume(map->chunk_radius);

    for (size_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
        GameHexChunkKey chunk_key = map->chunks[chunk_index].key;
        for (uint32_t local_index = 0u; local_index < tile_count_per_chunk; ++local_index) {
            GameHexChunkLocal local = {0};
            world_map_decompose_local(local_index, map->chunk_radius, &local);

            int32_t cube_x = chunk_key.cx * map->chunk_radius + local.lx;
            int32_t cube_z = chunk_key.cz * map->chunk_radius + local.lz;
            int32_t cube_y = -(cube_x + cube_z);
            int32_t expected_ly = cube_y - chunk_key.cy * map->chunk_radius;
            if (expected_ly != local.ly) {
                continue;
            }

            GameWorldMapTileVisit visit = {
                .chunk_key = chunk_key,
                .local = local,
                .local_index = local_index,
                .tile = game_hex_cube_to_axial((GameHexCube){cube_x, cube_y, cube_z}),
                .value = map->chunks[chunk_index].tile_values[local_index],
            };

            visitor(user, visit);
        }
    }

    return GAME_WORLD_MAP_RESULT_OK;
}
