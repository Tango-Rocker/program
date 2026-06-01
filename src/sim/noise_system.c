#include "sim/noise_system.h"

#include <limits.h>
#include <string.h>

#include "world/hex.h"

static int32_t game_noise_attenuate(GameCommandNoisePayload noise, int32_t distance) {
    int64_t attenuation = (int64_t)noise.attenuation * (int64_t)distance;
    int64_t remaining = (int64_t)noise.intensity - attenuation;
    return remaining > 0 ? (int32_t)remaining : 0;
}

static GameNoiseResult game_noise_emit_event(
    const GameNoiseEmittedPayload *payload,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    uint64_t *out_event_sequence
) {
    if (!payload || !out_event_sequence) {
        return GAME_NOISE_RESULT_INVALID_ARGUMENT;
    }

    GameEvent event = {
        .type = GAME_EVENT_TYPE_NOISE_EMITTED,
        .tick = current_tick,
        .source = {payload->source_index, payload->source_generation},
        .payload_size = sizeof(GameNoiseEmittedPayload),
    };
    memcpy(&event.payload, payload, sizeof(*payload));
    if (event_queue && game_event_queue_push(event_queue, &event) == GAME_EVENT_QUEUE_RESULT_OK) {
        // continue
    } else if (event_queue) {
        return GAME_NOISE_RESULT_EVENT_QUEUE_FULL;
    }

    if (event_log
        && game_event_log_append(
               event_log,
               current_tick,
               GAME_EVENT_TYPE_NOISE_EMITTED,
               payload->source_index == UINT32_MAX ? (GameEntityId){0, 0} : (GameEntityId){payload->source_index, payload->source_generation},
               GAME_EVENT_LOG_INVALID_PARENT_ID,
               out_event_sequence
           ) != GAME_EVENT_LOG_RESULT_OK) {
        return GAME_NOISE_RESULT_EVENT_LOG_FULL;
    }

    return GAME_NOISE_RESULT_OK;
}

GameNoiseResult game_noise_decode_command_payload(const GameCommand *command, GameNoiseEmittedPayload *out_payload) {
    if (!command || !out_payload) {
        return GAME_NOISE_RESULT_INVALID_ARGUMENT;
    }

    if (command->type != GAME_COMMAND_TYPE_EMIT_NOISE) {
        return GAME_NOISE_RESULT_NOT_NOISE_COMMAND;
    }

    if (command->payload_size != sizeof(GameCommandNoisePayload)) {
        return GAME_NOISE_RESULT_BAD_PAYLOAD;
    }

    GameCommandNoisePayload parsed = {0};
    memcpy(&parsed, command->payload.bytes, sizeof(parsed));

    if (parsed.max_radius <= 0) {
        return GAME_NOISE_RESULT_BAD_PAYLOAD;
    }

    out_payload->origin_q = parsed.origin_q;
    out_payload->origin_r = parsed.origin_r;
    out_payload->intensity = parsed.intensity;
    out_payload->max_radius = parsed.max_radius;
    out_payload->attenuation = parsed.attenuation;
    out_payload->decay_ticks = parsed.decay_ticks;
    out_payload->source_index = command->source.index;
    out_payload->source_generation = command->source.generation;
    out_payload->source_tick = command->requested_tick;

    return GAME_NOISE_RESULT_OK;
}

GameNoiseResult game_noise_emit_from_command(
    const GameCommand *command,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    GameTileField *field,
    GameNoiseAppliedResult *out_result
) {
    if (!field || !out_result) {
        return GAME_NOISE_RESULT_INVALID_ARGUMENT;
    }

    GameNoiseEmittedPayload payload = {0};
    GameNoiseResult decode = game_noise_decode_command_payload(command, &payload);
    if (decode != GAME_NOISE_RESULT_OK) {
        return decode;
    }

    *out_result = (GameNoiseAppliedResult){0};

    uint64_t noise_seq = 0u;
    GameNoiseResult emit = game_noise_emit_event(&payload, current_tick, event_queue, event_log, &noise_seq);
    if (emit != GAME_NOISE_RESULT_OK) {
        return emit;
    }
    out_result->noise_event_seq = noise_seq;

    int64_t total_added = 0;
    for (int32_t q = 0; q < (int32_t)field->config.q_count; ++q) {
        for (int32_t r = 0; r < (int32_t)field->config.r_count; ++r) {
            GameHexAxial tile = {
                .q = field->config.q_min + q,
                .r = field->config.r_min + r,
            };
            int32_t distance = game_hex_axial_distance((GameHexAxial){payload.origin_q, payload.origin_r}, tile);
            if (distance > payload.max_radius) {
                out_result->skipped_tiles++;
                continue;
            }

            int32_t apply = game_noise_attenuate((GameCommandNoisePayload){
                .origin_q = payload.origin_q,
                .origin_r = payload.origin_r,
                .intensity = payload.intensity,
                .max_radius = payload.max_radius,
                .attenuation = payload.attenuation,
                .decay_ticks = payload.decay_ticks,
            }, distance);

            if (apply <= 0) {
                out_result->skipped_tiles++;
                continue;
            }

            GameTileFieldResult add_result = game_tile_field_add(field, tile, apply);
            if (add_result != GAME_TILE_FIELD_RESULT_OK) {
                out_result->skipped_tiles++;
                continue;
            }
            total_added += apply;
            out_result->updated_tiles++;
        }
    }

    GameNoiseFieldImpulsePayload field_payload = {
        .origin_q = payload.origin_q,
        .origin_r = payload.origin_r,
        .max_radius = payload.max_radius,
        .updated_tiles = (int32_t)out_result->updated_tiles,
        .skipped_tiles = (int32_t)out_result->skipped_tiles,
        .total_added = (int32_t)(total_added > INT32_MAX ? INT32_MAX : total_added),
        .parent_event_sequence = noise_seq,
    };

    if (out_result->updated_tiles > 0 && event_queue) {
        GameEvent field_event = {
            .type = GAME_EVENT_TYPE_FIELD_IMPULSE_APPLIED,
            .tick = current_tick,
            .source = {payload.source_index, payload.source_generation},
            .payload_size = sizeof(GameNoiseFieldImpulsePayload),
        };
        memcpy(&field_event.payload, &field_payload, sizeof(field_payload));

        if (game_event_queue_push(event_queue, &field_event) != GAME_EVENT_QUEUE_RESULT_OK) {
            return GAME_NOISE_RESULT_EVENT_QUEUE_FULL;
        }
    }

    if (event_log && out_result->updated_tiles > 0) {
        if (game_event_log_append(
                event_log,
                current_tick,
                GAME_EVENT_TYPE_FIELD_IMPULSE_APPLIED,
                (GameEntityId){payload.source_index, payload.source_generation},
                noise_seq,
                &out_result->field_event_seq
            ) != GAME_EVENT_LOG_RESULT_OK) {
            return GAME_NOISE_RESULT_EVENT_LOG_FULL;
        }
        out_result->field_event_appended = true;
    }

    return GAME_NOISE_RESULT_OK;
}
