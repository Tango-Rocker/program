#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "world/hex.h"
#include "world/world_map.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_HEX_DEBUG_RENDER_RESULT_OK = 0,
    GAME_HEX_DEBUG_RENDER_RESULT_INVALID_ARGUMENT = 1,
    GAME_HEX_DEBUG_RENDER_RESULT_BUFFER_TOO_SMALL = 2,
} GameHexDebugRenderResult;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} GameDebugColor;

typedef struct {
    GameHexAxial tile;
    int32_t tile_value;
    GameDebugColor color;
    bool blocked;
    bool selected;
    bool highlighted;
} GameHexDebugCell;

GameDebugColor game_hex_debug_color_for_tile(int32_t tile_value, bool selected, bool highlighted);
GameHexDebugRenderResult game_hex_debug_render_collect(
    const GameWorldMap *map,
    GameHexAxial selected,
    bool has_selected,
    GameHexDebugCell *out_cells,
    size_t capacity,
    size_t *out_count
);

#ifdef __cplusplus
}
#endif
