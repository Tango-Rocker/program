#pragma once

#include <stddef.h>
#include <stdint.h>

#include "world/field_registry.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_FIELD_OVERLAY_RESULT_OK = 0,
    GAME_FIELD_OVERLAY_RESULT_INVALID_ARGUMENT = 1,
    GAME_FIELD_OVERLAY_RESULT_FIELD_NOT_FOUND = 2,
    GAME_FIELD_OVERLAY_RESULT_BUFFER_TOO_SMALL = 3,
} GameFieldOverlayResult;

typedef struct {
    GameFieldId field_id;
    GameHexAxial tile;
    int32_t value;
    uint8_t bucket;
} GameFieldOverlayCell;

typedef struct {
    GameHexAxial min;
    GameHexAxial max;
} GameFieldOverlayBounds;

GameFieldOverlayResult game_field_overlay_extract(
    const GameFieldRegistry *registry,
    GameFieldId field_id,
    GameFieldOverlayBounds bounds,
    GameFieldOverlayCell *out_cells,
    size_t capacity,
    size_t *out_count
);

#ifdef __cplusplus
}
#endif
