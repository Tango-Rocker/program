#include "sim/horde_attention.h"

#include <string.h>

static GameHordePosture game_horde_attention_next_posture(uint32_t pressure, uint32_t attack_threshold, uint32_t calm_threshold, GameHordePosture current) {
    if (pressure >= attack_threshold) {
        return GAME_HORDE_POSTURE_THREATENED;
    }
    if (pressure <= calm_threshold) {
        return GAME_HORDE_POSTURE_DORMANT;
    }

    return current;
}

void game_horde_attention_init(GameHordeAttentionState *state, const GameHordeAttentionConfig *config) {
    if (!state || !config) {
        return;
    }

    state->config = *config;
    state->posture = GAME_HORDE_POSTURE_DORMANT;
    state->last_update_tick = 0u;
    state->pressure = 0u;
    state->last_source = (GameEntityId){UINT32_MAX, UINT32_MAX};
    state->last_source_age_ticks = 0u;
}

static GameHordeAttentionResult game_horde_append_attention_event(
    GameHordeAttentionState *state,
    GameEntityId source,
    uint64_t tick,
    uint64_t parent_sequence,
    GameEventLog *event_log,
    uint64_t *out_sequence
) {
    if (!event_log) {
        return GAME_HORDE_ATTENTION_RESULT_OK;
    }

    if (game_event_log_append(
            event_log,
            tick,
            GAME_EVENT_TYPE_ACTOR_ATTENTION_UPDATED,
            source,
            parent_sequence,
            out_sequence
        ) != GAME_EVENT_LOG_RESULT_OK) {
        return GAME_HORDE_ATTENTION_RESULT_EVENT_LOG_FULL;
    }

    return GAME_HORDE_ATTENTION_RESULT_OK;
}

GameHordeAttentionResult game_horde_attention_tick(
    GameHordeAttentionState *state,
    uint32_t noise_field_sample,
    GameEntityId noise_source,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    GameHordeAttentionUpdateResult *out_result
) {
    if (!state || !out_result) {
        return GAME_HORDE_ATTENTION_RESULT_INVALID_ARGUMENT;
    }

    GameHordeAttentionUpdateResult result = {0};
    result.previous_pressure = state->pressure;
    result.previous_posture = state->posture;

    uint64_t elapsed = (state->last_update_tick == 0u) ? 0u : (tick - state->last_update_tick);
    state->last_update_tick = tick;

    for (uint64_t i = 0; i < elapsed; ++i) {
        if (state->pressure > 0u) {
            uint32_t decay = state->config.decay_per_tick;
            if (decay >= state->pressure) {
                state->pressure = 0u;
            } else {
                state->pressure -= decay;
            }
        }
    }

    if (noise_field_sample > 0u) {
        uint32_t rise = state->config.pressurize_per_sample * noise_field_sample;
        if (UINT32_MAX - state->pressure < rise) {
            state->pressure = state->config.max_pressure;
        } else {
            state->pressure += rise;
        }
        state->last_source = noise_source;
        state->last_source_age_ticks = 0u;
    } else {
        state->last_source_age_ticks += 1u;
    }

    if (state->pressure > state->config.max_pressure) {
        state->pressure = state->config.max_pressure;
    }

    result.current_pressure = state->pressure;
    result.current_posture = game_horde_attention_next_posture(state->pressure, state->config.attack_threshold, state->config.calm_threshold, state->posture);

    if (result.current_posture != result.previous_posture || result.current_pressure != result.previous_pressure) {
        result.emitted = true;
        if (game_horde_append_attention_event(
                state,
                state->last_source,
                tick,
                parent_event_sequence,
                event_log,
                &result.emitted_event_seq
            ) != GAME_HORDE_ATTENTION_RESULT_OK) {
            *out_result = result;
            return GAME_HORDE_ATTENTION_RESULT_EVENT_LOG_FULL;
        }
    }

    state->posture = result.current_posture;
    *out_result = result;
    return GAME_HORDE_ATTENTION_RESULT_OK;
}
