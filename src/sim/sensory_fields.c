#include "sim/sensory_fields.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "world/tile_field.h"

static GameSensoryFieldsResult game_sensory_fields_validate(GameFieldRegistry *registry, GameFieldId id, GameTileField **out_field) {
    if (!registry || !out_field) {
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }

    GameFieldRegistryResult result = game_field_registry_get(registry, id, out_field);
    if (result == GAME_FIELD_REGISTRY_RESULT_NOT_FOUND) {
        return GAME_SENSORY_FIELDS_RESULT_FIELD_NOT_FOUND;
    }
    if (result == GAME_FIELD_REGISTRY_RESULT_DISABLED) {
        return GAME_SENSORY_FIELDS_RESULT_FIELD_DISABLED;
    }
    if (result != GAME_FIELD_REGISTRY_RESULT_OK) {
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }
    return GAME_SENSORY_FIELDS_RESULT_OK;
}

static GameSensoryFieldsResult game_sensory_fields_validate_const(
    const GameFieldRegistry *registry,
    GameFieldId id,
    const GameTileField **out_field
) {
    if (!registry || !out_field) {
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }

    const GameTileField *const_field = NULL;
    GameFieldRegistryResult result = game_field_registry_get_const(registry, id, &const_field);
    if (result == GAME_FIELD_REGISTRY_RESULT_NOT_FOUND) {
        return GAME_SENSORY_FIELDS_RESULT_FIELD_NOT_FOUND;
    }
    if (result == GAME_FIELD_REGISTRY_RESULT_DISABLED) {
        return GAME_SENSORY_FIELDS_RESULT_FIELD_DISABLED;
    }
    if (result != GAME_FIELD_REGISTRY_RESULT_OK) {
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }
    *out_field = const_field;
    return GAME_SENSORY_FIELDS_RESULT_OK;
}

static bool game_sensory_fields_validate_impulse_args(int32_t intensity, int32_t max_radius, int32_t attenuation) {
    return intensity > 0 && max_radius > 0 && attenuation > 0;
}

static int32_t game_sensory_light_delta(int32_t intensity, int32_t distance, int32_t attenuation) {
    int64_t delta = (int64_t)intensity - ((int64_t)attenuation * (int64_t)distance);
    if (delta < 0) {
        return 0;
    }
    return (int32_t)delta;
}

static int32_t game_sensory_scent_delta(int32_t intensity, int32_t distance, int32_t attenuation) {
    int64_t delta = (int64_t)intensity - (int64_t)attenuation * (int64_t)distance * 2;
    if (delta < 0) {
        return 0;
    }
    return (int32_t)delta;
}

static int32_t game_sensory_blood_delta(int32_t intensity, int32_t distance, int32_t attenuation) {
    if (distance == 0) {
        return intensity;
    }

    int64_t delta = (int64_t)intensity - ((int64_t)attenuation * (int64_t)distance * 2);
    if (delta < 0) {
        return 0;
    }
    return (int32_t)delta;
}

static int32_t game_sensory_apply_impulse_to_field(
    GameFieldRegistry *registry,
    GameFieldId field_id,
    GameHexAxial origin,
    int32_t intensity,
    int32_t max_radius,
    int32_t attenuation,
    int32_t (*compute_delta)(int32_t intensity, int32_t distance, int32_t attenuation),
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    GameSensoryFieldImpulseResult *out_result
) {
    if (!compute_delta || !out_result) {
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }

    if (!game_sensory_fields_validate_impulse_args(intensity, max_radius, attenuation)) {
        *out_result = (GameSensoryFieldImpulseResult){0};
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }

    *out_result = (GameSensoryFieldImpulseResult){0};
    GameTileField *field = NULL;
    GameSensoryFieldsResult validation = game_sensory_fields_validate(registry, field_id, &field);
    if (validation != GAME_SENSORY_FIELDS_RESULT_OK) {
        return validation;
    }

    int64_t total_added = 0;
    for (int32_t q = 0; q < (int32_t)field->config.q_count; ++q) {
        for (int32_t r = 0; r < (int32_t)field->config.r_count; ++r) {
            GameHexAxial tile = {
                .q = field->config.q_min + q,
                .r = field->config.r_min + r,
            };

            int32_t distance = game_hex_axial_distance(origin, tile);
            if (distance > max_radius) {
                ++out_result->skipped_tiles;
                continue;
            }

            int32_t delta = compute_delta(intensity, distance, attenuation);
            if (delta <= 0) {
                ++out_result->skipped_tiles;
                continue;
            }

            GameTileFieldResult add_result = game_tile_field_add(field, tile, delta);
            if (add_result != GAME_TILE_FIELD_RESULT_OK) {
                ++out_result->skipped_tiles;
                continue;
            }
            ++out_result->updated_tiles;
            total_added += delta;
        }
    }

    out_result->total_added = (int32_t)(total_added < INT32_MIN ? INT32_MIN : (total_added > INT32_MAX ? INT32_MAX : total_added));
    if (out_result->updated_tiles == 0u) {
        return GAME_SENSORY_FIELDS_RESULT_OK;
    }

    if (!event_log) {
        return GAME_SENSORY_FIELDS_RESULT_OK;
    }

    if (game_event_log_append(
            event_log,
            tick,
            GAME_EVENT_TYPE_FIELD_IMPULSE_APPLIED,
            (GameEntityId){(uint32_t)field_id, 0u},
            parent_event_sequence,
            &out_result->traced_event_sequence
        ) != GAME_EVENT_LOG_RESULT_OK) {
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }
    out_result->event_appended = true;
    return GAME_SENSORY_FIELDS_RESULT_OK;
}

GameSensoryFieldsResult game_sensory_apply_light_impulse(
    GameFieldRegistry *registry,
    GameHexAxial origin,
    int32_t intensity,
    int32_t max_radius,
    int32_t attenuation,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    GameSensoryFieldImpulseResult *out_result
) {
    return game_sensory_apply_impulse_to_field(
        registry,
        GAME_FIELD_ID_LIGHT,
        origin,
        intensity,
        max_radius,
        attenuation,
        game_sensory_light_delta,
        tick,
        parent_event_sequence,
        event_log,
        out_result
    );
}

GameSensoryFieldsResult game_sensory_apply_scent_impulse(
    GameFieldRegistry *registry,
    GameHexAxial origin,
    int32_t intensity,
    int32_t max_radius,
    int32_t attenuation,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    GameSensoryFieldImpulseResult *out_result
) {
    return game_sensory_apply_impulse_to_field(
        registry,
        GAME_FIELD_ID_SCENT,
        origin,
        intensity,
        max_radius,
        attenuation,
        game_sensory_scent_delta,
        tick,
        parent_event_sequence,
        event_log,
        out_result
    );
}

GameSensoryFieldsResult game_sensory_apply_blood_impulse(
    GameFieldRegistry *registry,
    GameHexAxial origin,
    int32_t intensity,
    int32_t max_radius,
    int32_t attenuation,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    GameSensoryFieldImpulseResult *out_result
) {
    return game_sensory_apply_impulse_to_field(
        registry,
        GAME_FIELD_ID_BLOOD,
        origin,
        intensity,
        max_radius,
        attenuation,
        game_sensory_blood_delta,
        tick,
        parent_event_sequence,
        event_log,
        out_result
    );
}

static GameSensoryFieldsResult game_sensory_decay_field(GameFieldRegistry *registry, GameFieldId id, int32_t decay_per_tick) {
    if (!registry || decay_per_tick < 0) {
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }
    if (decay_per_tick == 0) {
        return GAME_SENSORY_FIELDS_RESULT_OK;
    }

    GameTileField *field = NULL;
    GameSensoryFieldsResult validation = game_sensory_fields_validate(registry, id, &field);
    if (validation != GAME_SENSORY_FIELDS_RESULT_OK) {
        return validation;
    }

    return game_tile_field_decay(field, decay_per_tick) == GAME_TILE_FIELD_RESULT_OK ? GAME_SENSORY_FIELDS_RESULT_OK
                                                                                : GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
}

GameSensoryFieldsResult game_sensory_decay_light_field(GameFieldRegistry *registry, int32_t decay_per_tick, int32_t *out_affected_tiles) {
    if (out_affected_tiles == NULL) {
        return game_sensory_decay_field(registry, GAME_FIELD_ID_LIGHT, decay_per_tick);
    }

    GameTileField *field = NULL;
    GameSensoryFieldsResult validation = game_sensory_fields_validate(registry, GAME_FIELD_ID_LIGHT, &field);
    if (validation != GAME_SENSORY_FIELDS_RESULT_OK) {
        return validation;
    }

    *out_affected_tiles = (int32_t)(field->capacity > INT32_MAX ? INT32_MAX : field->capacity);
    return game_sensory_decay_field(registry, GAME_FIELD_ID_LIGHT, decay_per_tick);
}

GameSensoryFieldsResult game_sensory_decay_scent_field(GameFieldRegistry *registry, int32_t decay_per_tick, int32_t *out_affected_tiles) {
    if (out_affected_tiles == NULL) {
        return game_sensory_decay_field(registry, GAME_FIELD_ID_SCENT, decay_per_tick);
    }

    GameTileField *field = NULL;
    GameSensoryFieldsResult validation = game_sensory_fields_validate(registry, GAME_FIELD_ID_SCENT, &field);
    if (validation != GAME_SENSORY_FIELDS_RESULT_OK) {
        return validation;
    }

    *out_affected_tiles = (int32_t)(field->capacity > INT32_MAX ? INT32_MAX : field->capacity);
    return game_sensory_decay_field(registry, GAME_FIELD_ID_SCENT, decay_per_tick);
}

GameSensoryFieldsResult game_sensory_decay_blood_field(GameFieldRegistry *registry, int32_t decay_per_tick, int32_t *out_affected_tiles) {
    if (out_affected_tiles == NULL) {
        return game_sensory_decay_field(registry, GAME_FIELD_ID_BLOOD, decay_per_tick);
    }

    GameTileField *field = NULL;
    GameSensoryFieldsResult validation = game_sensory_fields_validate(registry, GAME_FIELD_ID_BLOOD, &field);
    if (validation != GAME_SENSORY_FIELDS_RESULT_OK) {
        return validation;
    }

    *out_affected_tiles = (int32_t)(field->capacity > INT32_MAX ? INT32_MAX : field->capacity);
    return game_sensory_decay_field(registry, GAME_FIELD_ID_BLOOD, decay_per_tick);
}

GameSensoryFieldsResult game_sensory_sample_field(
    const GameFieldRegistry *registry,
    GameFieldId field_id,
    GameHexAxial position,
    int32_t *out_value
) {
    if (!out_value) {
        return GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
    }
    const GameTileField *field = NULL;
    GameSensoryFieldsResult validation = game_sensory_fields_validate_const(registry, field_id, &field);
    if (validation != GAME_SENSORY_FIELDS_RESULT_OK) {
        return validation;
    }

    return game_tile_field_get((GameTileField *)field, position, out_value) == GAME_TILE_FIELD_RESULT_OK
               ? GAME_SENSORY_FIELDS_RESULT_OK
               : GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT;
}
