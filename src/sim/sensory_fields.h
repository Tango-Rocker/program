#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "event/event.h"
#include "event/event_log.h"
#include "world/field_registry.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_SENSORY_FIELDS_RESULT_OK = 0,
    GAME_SENSORY_FIELDS_RESULT_INVALID_ARGUMENT = 1,
    GAME_SENSORY_FIELDS_RESULT_FIELD_NOT_FOUND = 2,
    GAME_SENSORY_FIELDS_RESULT_FIELD_DISABLED = 3,
} GameSensoryFieldsResult;

typedef struct {
    uint32_t updated_tiles;
    uint32_t skipped_tiles;
    int32_t total_added;
    uint64_t traced_event_sequence;
    bool event_appended;
} GameSensoryFieldImpulseResult;

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
);

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
);

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
);

GameSensoryFieldsResult game_sensory_decay_light_field(GameFieldRegistry *registry, int32_t decay_per_tick, int32_t *out_affected_tiles);
GameSensoryFieldsResult game_sensory_decay_scent_field(GameFieldRegistry *registry, int32_t decay_per_tick, int32_t *out_affected_tiles);
GameSensoryFieldsResult game_sensory_decay_blood_field(GameFieldRegistry *registry, int32_t decay_per_tick, int32_t *out_affected_tiles);

GameSensoryFieldsResult game_sensory_sample_field(const GameFieldRegistry *registry, GameFieldId field_id, GameHexAxial position, int32_t *out_value);

#ifdef __cplusplus
}
#endif
