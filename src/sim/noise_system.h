#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "event/event.h"
#include "event/event_log.h"
#include "sim/command.h"
#include "world/tile_field.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GameNoiseAppliedResult {
    uint32_t updated_tiles;
    uint32_t skipped_tiles;
    uint64_t noise_event_seq;
    uint64_t field_event_seq;
    bool field_event_appended;
} GameNoiseAppliedResult;

typedef enum {
    GAME_NOISE_RESULT_OK = 0,
    GAME_NOISE_RESULT_INVALID_ARGUMENT = 1,
    GAME_NOISE_RESULT_NOT_NOISE_COMMAND = 2,
    GAME_NOISE_RESULT_BAD_PAYLOAD = 3,
    GAME_NOISE_RESULT_EVENT_LOG_FULL = 4,
    GAME_NOISE_RESULT_EVENT_QUEUE_FULL = 5,
} GameNoiseResult;

typedef struct GameNoiseEmittedPayload {
    int32_t origin_q;
    int32_t origin_r;
    int32_t intensity;
    int32_t max_radius;
    int32_t attenuation;
    uint64_t source_tick;
    uint32_t decay_ticks;
    uint32_t source_index;
    uint32_t source_generation;
} GameNoiseEmittedPayload;

typedef struct GameNoiseFieldImpulsePayload {
    int32_t origin_q;
    int32_t origin_r;
    int32_t max_radius;
    int32_t updated_tiles;
    int32_t skipped_tiles;
    int32_t total_added;
    uint64_t parent_event_sequence;
} GameNoiseFieldImpulsePayload;

GameNoiseResult game_noise_decode_command_payload(const GameCommand *command, GameNoiseEmittedPayload *out_payload);
GameNoiseResult game_noise_emit_from_command(
    const GameCommand *command,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    GameTileField *field,
    GameNoiseAppliedResult *out_result
);

#ifdef __cplusplus
}
#endif
