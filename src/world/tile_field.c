#include "world/tile_field.h"

#include <limits.h>
#include <stdlib.h>

static size_t game_tile_field_index(const GameTileField *field, GameHexAxial position) {
    int64_t q_offset = (int64_t)position.q - field->config.q_min;
    int64_t r_offset = (int64_t)position.r - field->config.r_min;
    return (size_t)(q_offset * (int64_t)field->config.r_count + r_offset);
}

static GameTileFieldResult game_tile_field_clamp(GameTileField *field, int32_t *value) {
    if (!field || !value) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    if (*value < field->config.min_value) {
        *value = field->config.min_value;
        return GAME_TILE_FIELD_RESULT_OK;
    }

    if (*value > field->config.max_value) {
        *value = field->config.max_value;
    }

    return GAME_TILE_FIELD_RESULT_OK;
}

static GameTileFieldResult game_tile_field_check_bounds(const GameTileField *field, GameHexAxial position, size_t *out_index) {
    if (!field || !field->values) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    int64_t q_offset = (int64_t)position.q - field->config.q_min;
    int64_t r_offset = (int64_t)position.r - field->config.r_min;
    if (q_offset < 0 || r_offset < 0) {
        return GAME_TILE_FIELD_RESULT_OUT_OF_BOUNDS;
    }

    if (q_offset >= (int64_t)field->config.q_count || r_offset >= (int64_t)field->config.r_count) {
        return GAME_TILE_FIELD_RESULT_OUT_OF_BOUNDS;
    }

    if (out_index) {
        *out_index = (size_t)q_offset * field->config.r_count + (size_t)r_offset;
    }

    return GAME_TILE_FIELD_RESULT_OK;
}

GameTileFieldResult game_tile_field_init(GameTileField *field, const GameTileFieldConfig *config) {
    if (!field || !config || config->q_count == 0 || config->r_count == 0) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    if (config->min_value > config->max_value) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    if (config->q_count > SIZE_MAX / config->r_count) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    size_t capacity = config->q_count * config->r_count;
    int32_t *values = (int32_t *)calloc(capacity, sizeof(int32_t));
    if (!values) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    field->values = values;
    field->capacity = capacity;
    field->config = *config;

    return GAME_TILE_FIELD_RESULT_OK;
}

void game_tile_field_destroy(GameTileField *field) {
    if (!field) {
        return;
    }

    free(field->values);
    field->values = NULL;
    field->capacity = 0u;
    field->config = (GameTileFieldConfig){0};
}

GameTileFieldResult game_tile_field_clear(GameTileField *field, int32_t value) {
    if (!field || !field->values) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < field->capacity; ++i) {
        field->values[i] = value;
    }

    return GAME_TILE_FIELD_RESULT_OK;
}

GameTileFieldResult game_tile_field_get(GameTileField *field, GameHexAxial position, int32_t *out_value) {
    if (!out_value) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    size_t index = 0u;
    GameTileFieldResult result = game_tile_field_check_bounds(field, position, &index);
    if (result != GAME_TILE_FIELD_RESULT_OK) {
        return result;
    }

    *out_value = field->values[index];
    return GAME_TILE_FIELD_RESULT_OK;
}

GameTileFieldResult game_tile_field_set(GameTileField *field, GameHexAxial position, int32_t value) {
    size_t index = 0u;
    GameTileFieldResult result = game_tile_field_check_bounds(field, position, &index);
    if (result != GAME_TILE_FIELD_RESULT_OK) {
        return result;
    }

    if (game_tile_field_clamp(field, &value) != GAME_TILE_FIELD_RESULT_OK) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    field->values[index] = value;
    return GAME_TILE_FIELD_RESULT_OK;
}

GameTileFieldResult game_tile_field_add(GameTileField *field, GameHexAxial position, int32_t delta) {
    if (!field || !field->values) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    size_t index = 0u;
    GameTileFieldResult result = game_tile_field_check_bounds(field, position, &index);
    if (result != GAME_TILE_FIELD_RESULT_OK) {
        return result;
    }

    int64_t next = (int64_t)field->values[index] + (int64_t)delta;
    int32_t next_value = (int32_t)next;
    game_tile_field_clamp(field, &next_value);
    field->values[index] = next_value;
    return GAME_TILE_FIELD_RESULT_OK;
}

GameTileFieldResult game_tile_field_decay(GameTileField *field, int32_t decay) {
    if (!field || !field->values) {
        return GAME_TILE_FIELD_RESULT_INVALID_ARGUMENT;
    }

    if (decay <= 0) {
        return GAME_TILE_FIELD_RESULT_OK;
    }

    for (size_t i = 0; i < field->capacity; ++i) {
        if (field->values[i] > 0) {
            int64_t next = (int64_t)field->values[i] - (int64_t)decay;
            int32_t next_value = (next < 0) ? 0 : (int32_t)next;
            game_tile_field_clamp(field, &next_value);
            field->values[i] = next_value;
        } else if (field->values[i] < 0) {
            int64_t next = (int64_t)field->values[i] + (int64_t)decay;
            int32_t next_value = (next > 0) ? 0 : (int32_t)next;
            game_tile_field_clamp(field, &next_value);
            field->values[i] = next_value;
        }
    }

    return GAME_TILE_FIELD_RESULT_OK;
}

size_t game_tile_field_count(const GameTileField *field) {
    return field ? field->capacity : 0u;
}

bool game_tile_field_contains(const GameTileField *field, GameHexAxial position) {
    return game_tile_field_check_bounds(field, position, NULL) == GAME_TILE_FIELD_RESULT_OK;
}
