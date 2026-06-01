#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_TILE_FIELD_RESULT_OK = 0,
    GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT = 1,
    GAME_TILE_FIELD_RESULT_OUT_OF_BOUNDS = 2,
    GAME_TILE_FIELD_RESULT_FULL = 3,
} GameTileFieldResult;

typedef struct GameTileFieldConfig {
    int32_t q_min;
    int32_t r_min;
    size_t q_count;
    size_t r_count;
    int32_t min_value;
    int32_t max_value;
} GameTileFieldConfig;

typedef struct GameTileField {
    GameTileFieldConfig config;
    int32_t *values;
    size_t capacity;
} GameTileField;

GameTileFieldResult game_tile_field_init(GameTileField *field, const GameTileFieldConfig *config);
void game_tile_field_destroy(GameTileField *field);
GameTileFieldResult game_tile_field_clear(GameTileField *field, int32_t value);
GameTileFieldResult game_tile_field_get(GameTileField *field, GameHexAxial position, int32_t *out_value);
GameTileFieldResult game_tile_field_set(GameTileField *field, GameHexAxial position, int32_t value);
GameTileFieldResult game_tile_field_add(GameTileField *field, GameHexAxial position, int32_t delta);
GameTileFieldResult game_tile_field_decay(GameTileField *field, int32_t decay);
size_t game_tile_field_count(const GameTileField *field);
bool game_tile_field_contains(const GameTileField *field, GameHexAxial position);

#ifdef __cplusplus
}
#endif
