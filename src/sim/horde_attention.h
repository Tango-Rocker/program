#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_HORDE_POSTURE_DORMANT = 0,
    GAME_HORDE_POSTURE_ALERT = 1,
    GAME_HORDE_POSTURE_THREATENED = 2,
} GameHordePosture;

typedef struct GameHordeAttentionConfig {
    uint32_t pressurize_per_sample;
    uint32_t decay_per_tick;
    uint32_t attack_threshold;
    uint32_t calm_threshold;
    uint32_t max_pressure;
} GameHordeAttentionConfig;

typedef struct GameHordeAttentionState {
    GameHordeAttentionConfig config;
    GameHordePosture posture;
    uint64_t last_update_tick;
    uint32_t pressure;
    GameEntityId last_source;
    uint64_t last_source_age_ticks;
} GameHordeAttentionState;

typedef enum {
    GAME_HORDE_ATTENTION_RESULT_OK = 0,
    GAME_HORDE_ATTENTION_RESULT_INVALID_ARGUMENT = 1,
    GAME_HORDE_ATTENTION_RESULT_EVENT_LOG_FULL = 2,
} GameHordeAttentionResult;

typedef struct GameHordeAttentionUpdateResult {
    uint32_t previous_pressure;
    uint32_t current_pressure;
    GameHordePosture previous_posture;
    GameHordePosture current_posture;
    uint64_t emitted_event_seq;
    bool emitted;
} GameHordeAttentionUpdateResult;

void game_horde_attention_init(GameHordeAttentionState *state, const GameHordeAttentionConfig *config);

GameHordeAttentionResult game_horde_attention_tick(
    GameHordeAttentionState *state,
    uint32_t noise_field_sample,
    GameEntityId noise_source,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    GameHordeAttentionUpdateResult *out_result
);

#ifdef __cplusplus
}
#endif
