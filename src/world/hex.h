#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_HEX_CHUNK_RADIUS 16

typedef struct GameHexAxial {
    int32_t q;
    int32_t r;
} GameHexAxial;

typedef struct GameHexCube {
    int32_t x;
    int32_t y;
    int32_t z;
} GameHexCube;

typedef struct GameHexChunkKey {
    int32_t cx;
    int32_t cy;
    int32_t cz;
} GameHexChunkKey;

typedef struct GameHexChunkLocal {
    int32_t lx;
    int32_t ly;
    int32_t lz;
} GameHexChunkLocal;

GameHexCube game_hex_axial_to_cube(GameHexAxial axial);
GameHexAxial game_hex_cube_to_axial(GameHexCube cube);
void game_hex_axial_neighbors(GameHexAxial tile, GameHexAxial out_neighbors[6]);
int32_t game_hex_axial_distance(GameHexAxial a, GameHexAxial b);

GameHexChunkKey game_hex_cube_to_chunk_key(GameHexCube cube, int32_t chunk_radius);
GameHexChunkLocal game_hex_cube_to_chunk_local(GameHexCube cube, int32_t chunk_radius);
uint32_t game_hex_local_to_linear_index(GameHexChunkLocal local, int32_t chunk_radius);
