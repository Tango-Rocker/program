#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_WORLD_MAP_RESULT_OK = 0,
    GAME_WORLD_MAP_RESULT_INVALID_ARGUMENT = 1,
    GAME_WORLD_MAP_RESULT_OUT_OF_MEMORY = 2,
    GAME_WORLD_MAP_RESULT_CHUNK_MISSING = 3,
    GAME_WORLD_MAP_RESULT_CHUNK_EXISTS = 4,
} GameWorldMapResult;

typedef struct {
    GameHexChunkKey key;
    int32_t *tile_values;
} GameWorldMapChunk;

typedef struct {
    GameHexChunkKey chunk_key;
    GameHexChunkLocal local;
    uint32_t local_index;
    GameHexAxial tile;
    int32_t value;
} GameWorldMapTileVisit;

typedef void (*GameWorldMapTileVisitor)(void *user, GameWorldMapTileVisit visit);

typedef struct {
    int32_t chunk_radius;
    int32_t default_tile_value;
    GameWorldMapChunk *chunks;
    size_t chunk_count;
    size_t chunk_capacity;
} GameWorldMap;

GameWorldMapResult game_world_map_init(GameWorldMap *map, int32_t chunk_radius, int32_t default_tile_value);
void game_world_map_destroy(GameWorldMap *map);
size_t game_world_map_chunk_count(const GameWorldMap *map);

GameWorldMapResult game_world_map_create_chunk(GameWorldMap *map, GameHexChunkKey chunk_key);
GameWorldMapResult game_world_map_get(const GameWorldMap *map, GameHexAxial position, int32_t *out_value);
GameWorldMapResult game_world_map_set(GameWorldMap *map, GameHexAxial position, int32_t value);
GameWorldMapResult game_world_map_get_local(const GameWorldMap *map, GameHexChunkKey chunk_key, GameHexChunkLocal local, int32_t *out_value);
GameWorldMapResult game_world_map_set_local(GameWorldMap *map, GameHexChunkKey chunk_key, GameHexChunkLocal local, int32_t value);

GameWorldMapResult game_world_map_for_each_tile(const GameWorldMap *map, GameWorldMapTileVisitor visitor, void *user);

#ifdef __cplusplus
}
#endif
