#include "audio/audio_request.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sim/projectile.h"
#include "sim/status_effect.h"
#include "sim/noise_system.h"

static bool game_audio_request_queue_is_valid(const GameAudioRequestQueue *queue) {
    return queue && queue->entries != NULL && queue->capacity > 0u;
}

GameAudioRequestResult game_audio_request_init(GameAudioRequestQueue *queue, GameAudioRequest *entries, size_t capacity) {
    if (!queue || (!entries && capacity > 0u)) {
        return GAME_AUDIO_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    queue->entries = entries;
    queue->capacity = capacity;
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
    return GAME_AUDIO_REQUEST_RESULT_OK;
}

void game_audio_request_destroy(GameAudioRequestQueue *queue) {
    if (!queue) {
        return;
    }
    queue->entries = NULL;
    queue->capacity = 0u;
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
}

void game_audio_request_clear(GameAudioRequestQueue *queue) {
    if (!queue) {
        return;
    }
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
}

size_t game_audio_request_count(const GameAudioRequestQueue *queue) {
    return queue ? queue->count : 0u;
}

GameAudioRequestResult game_audio_request_push(GameAudioRequestQueue *queue, const GameAudioRequest *request) {
    if (!game_audio_request_queue_is_valid(queue) || !request) {
        return GAME_AUDIO_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    if (queue->count >= queue->capacity) {
        return GAME_AUDIO_REQUEST_RESULT_QUEUE_FULL;
    }

    queue->entries[queue->tail] = *request;
    queue->tail++;
    if (queue->tail >= queue->capacity) {
        queue->tail = 0u;
    }
    queue->count++;
    return GAME_AUDIO_REQUEST_RESULT_OK;
}

GameAudioRequestResult game_audio_request_pop(GameAudioRequestQueue *queue, GameAudioRequest *out_request) {
    if (!game_audio_request_queue_is_valid(queue) || !out_request) {
        return GAME_AUDIO_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    if (queue->count == 0u) {
        return GAME_AUDIO_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    *out_request = queue->entries[queue->head];
    queue->head++;
    if (queue->head >= queue->capacity) {
        queue->head = 0u;
    }
    queue->count--;
    return GAME_AUDIO_REQUEST_RESULT_OK;
}

GameAudioRequestResult game_audio_request_peek(const GameAudioRequestQueue *queue, GameAudioRequest *out_request) {
    if (!game_audio_request_queue_is_valid(queue) || !out_request) {
        return GAME_AUDIO_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    if (queue->count == 0u) {
        return GAME_AUDIO_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    *out_request = queue->entries[queue->head];
    return GAME_AUDIO_REQUEST_RESULT_OK;
}

GameAudioRequestResult game_audio_request_project_from_event(
    const GameEvent *event,
    uint64_t requested_tick,
    uint64_t source_event_sequence,
    GameAudioRequestQueue *queue
) {
    if (!event || !game_audio_request_queue_is_valid(queue)) {
        return GAME_AUDIO_REQUEST_RESULT_INVALID_ARGUMENT;
    }

    GameAudioRequest request = {
        .requested_tick = requested_tick,
        .source_event_sequence = source_event_sequence,
        .priority = 1u,
    };

    if (event->type == GAME_EVENT_TYPE_NOISE_EMITTED) {
        if (event->payload_size < sizeof(GameNoiseEmittedPayload)) {
            return GAME_AUDIO_REQUEST_RESULT_BAD_PAYLOAD;
        }
        const GameNoiseEmittedPayload *noise = (const GameNoiseEmittedPayload *)event->payload.bytes;
        request.sound_id = 1u;
        request.origin_q = noise->origin_q;
        request.origin_r = noise->origin_r;
        request.priority = 3u;
    } else if (event->type == GAME_EVENT_TYPE_PROJECTILE_IMPACT) {
        if (event->payload_size < sizeof(GameProjectileImpactPayload)) {
            return GAME_AUDIO_REQUEST_RESULT_BAD_PAYLOAD;
        }
        const GameProjectileImpactPayload *impact = (const GameProjectileImpactPayload *)event->payload.bytes;
        request.sound_id = 2u;
        request.origin_q = impact->target_q;
        request.origin_r = impact->target_r;
        request.priority = 4u;
    } else if (event->type == GAME_EVENT_TYPE_THREAT_STATE_CHANGED) {
        request.sound_id = 4u;
        request.origin_q = 0;
        request.origin_r = 0;
        request.priority = 2u;
    } else if (event->type == GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED || event->type == GAME_EVENT_TYPE_STATUS_EFFECT_EXPIRED) {
        if (event->payload_size < sizeof(GameStatusEffectPayload)) {
            return GAME_AUDIO_REQUEST_RESULT_BAD_PAYLOAD;
        }
        const GameStatusEffectPayload *status = (const GameStatusEffectPayload *)event->payload.bytes;
        request.sound_id = 3u;
        request.origin_q = status->target_q;
        request.origin_r = status->target_r;
        request.priority = 2u;
    } else {
        return GAME_AUDIO_REQUEST_RESULT_UNKNOWN_EVENT;
    }

    return game_audio_request_push(queue, &request);
}
