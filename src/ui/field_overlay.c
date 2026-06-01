#include "ui/field_overlay.h"

static uint8_t game_field_overlay_bucket(const GameTileField *field, int32_t value) {
    int32_t range = field->config.max_value - field->config.min_value;
    if (range <= 0) {
        return 0u;
    }
    int32_t normalized = ((value - field->config.min_value) * 255) / range;
    if (normalized < 0) {
        normalized = 0;
    }
    if (normalized > 255) {
        normalized = 255;
    }
    return (uint8_t)normalized;
}

GameFieldOverlayResult game_field_overlay_extract(
    const GameFieldRegistry *registry,
    GameFieldId field_id,
    GameFieldOverlayBounds bounds,
    GameFieldOverlayCell *out_cells,
    size_t capacity,
    size_t *out_count
) {
    if (!registry || !out_cells || !out_count || capacity == 0u || bounds.max.q < bounds.min.q
        || bounds.max.r < bounds.min.r) {
        return GAME_FIELD_OVERLAY_RESULT_INVALID_ARGUMENT;
    }

    const GameTileField *field = NULL;
    GameFieldRegistryResult result = game_field_registry_get_const(registry, field_id, &field);
    if (result == GAME_FIELD_REGISTRY_RESULT_NOT_FOUND || result == GAME_FIELD_REGISTRY_RESULT_DISABLED) {
        return GAME_FIELD_OVERLAY_RESULT_FIELD_NOT_FOUND;
    }
    if (result != GAME_FIELD_REGISTRY_RESULT_OK || !field || !field->values) {
        return GAME_FIELD_OVERLAY_RESULT_INVALID_ARGUMENT;
    }

    size_t count = 0u;
    for (int32_t r = bounds.min.r; r <= bounds.max.r; ++r) {
        for (int32_t q = bounds.min.q; q <= bounds.max.q; ++q) {
            GameHexAxial tile = {q, r};
            if (!game_tile_field_contains(field, tile)) {
                continue;
            }
            if (count >= capacity) {
                *out_count = count;
                return GAME_FIELD_OVERLAY_RESULT_BUFFER_TOO_SMALL;
            }

            size_t index = (size_t)(q - field->config.q_min) * field->config.r_count
                           + (size_t)(r - field->config.r_min);
            int32_t value = field->values[index];
            out_cells[count++] = (GameFieldOverlayCell){
                .field_id = field_id,
                .tile = tile,
                .value = value,
                .bucket = game_field_overlay_bucket(field, value),
            };
        }
    }

    *out_count = count;
    return GAME_FIELD_OVERLAY_RESULT_OK;
}
