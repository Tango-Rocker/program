#include "world/hex.h"

#include <stdint.h>

static int32_t game_hex_floor_div(int32_t value, int32_t divisor) {
    if (divisor <= 0) {
        return 0;
    }

    int32_t q = value / divisor;
    if ((value % divisor) != 0 && ((value < 0) != (divisor < 0))) {
        q--;
    }

    return q;
}

GameHexCube game_hex_axial_to_cube(GameHexAxial axial) {
    return (GameHexCube){axial.q, axial.r, -axial.q - axial.r};
}

GameHexAxial game_hex_cube_to_axial(GameHexCube cube) {
    return (GameHexAxial){cube.x, cube.y};
}

void game_hex_axial_neighbors(GameHexAxial tile, GameHexAxial out_neighbors[6]) {
    const int32_t q_offsets[6] = {1, 0, -1, -1, 0, 1};
    const int32_t r_offsets[6] = {0, 1, 1, 0, -1, -1};
    for (int i = 0; i < 6; ++i) {
        out_neighbors[i] = (GameHexAxial){tile.q + q_offsets[i], tile.r + r_offsets[i]};
    }
}

int32_t game_hex_axial_distance(GameHexAxial a, GameHexAxial b) {
    GameHexCube ac = game_hex_axial_to_cube(a);
    GameHexCube bc = game_hex_axial_to_cube(b);
    int32_t dx = ac.x - bc.x;
    int32_t dy = ac.y - bc.y;
    int32_t dz = ac.z - bc.z;

    int32_t ax = dx < 0 ? -dx : dx;
    int32_t ay = dy < 0 ? -dy : dy;
    int32_t az = dz < 0 ? -dz : dz;

    int32_t max1 = ax > ay ? ax : ay;
    return max1 > az ? max1 : az;
}

GameHexChunkKey game_hex_cube_to_chunk_key(GameHexCube cube, int32_t chunk_radius) {
    if (chunk_radius <= 0) {
        return (GameHexChunkKey){0, 0, 0};
    }

    return (GameHexChunkKey){
        game_hex_floor_div(cube.x, chunk_radius),
        game_hex_floor_div(cube.y, chunk_radius),
        game_hex_floor_div(cube.z, chunk_radius),
    };
}

GameHexChunkLocal game_hex_cube_to_chunk_local(GameHexCube cube, int32_t chunk_radius) {
    GameHexChunkKey chunk = game_hex_cube_to_chunk_key(cube, chunk_radius);
    return (GameHexChunkLocal){
        cube.x - chunk.cx * chunk_radius,
        cube.y - chunk.cy * chunk_radius,
        cube.z - chunk.cz * chunk_radius,
    };
}

uint32_t game_hex_local_to_linear_index(GameHexChunkLocal local, int32_t chunk_radius) {
    if (chunk_radius <= 0) {
        return 0u;
    }

    return (uint32_t)((uint32_t)local.lx * (uint32_t)chunk_radius * (uint32_t)chunk_radius
                      + (uint32_t)local.ly * (uint32_t)chunk_radius
                      + (uint32_t)local.lz);
}
