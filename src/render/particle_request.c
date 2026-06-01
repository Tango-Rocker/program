#include "render/particle_request.h"

#include <stdbool.h>
#include <string.h>

#include "sim/projectile.h"
#include "sim/status_effect.h"

static bool game_particle_request_queue_is_valid(const GameParticleRequestQueue *queue) {
    return queue && queue->entries != NULL && queue->capacity > 0u;
}

GameParticleRequestResult game_particle_request_init(GameParticleRequestQueue *queue, GameParticleRequest *entries, size_t capacity) {
    if (!queue || (!entries && capacity > 0u)) {
        return GAME_PARTICLE_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    queue->entries = entries;
    queue->capacity = capacity;
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
    return GAME_PARTICLE_REQUEST_RESULT_OK;
}

void game_particle_request_destroy(GameParticleRequestQueue *queue) {
    if (!queue) {
        return;
    }
    queue->entries = NULL;
    queue->capacity = 0u;
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
}

void game_particle_request_clear(GameParticleRequestQueue *queue) {
    if (!queue) {
        return;
    }
    queue->head = 0u;
    queue->tail = 0u;
    queue->count = 0u;
}

size_t game_particle_request_count(const GameParticleRequestQueue *queue) {
    return queue ? queue->count : 0u;
}

GameParticleRequestResult game_particle_request_push(GameParticleRequestQueue *queue, const GameParticleRequest *request) {
    if (!game_particle_request_queue_is_valid(queue) || !request) {
        return GAME_PARTICLE_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    if (queue->count >= queue->capacity) {
        return GAME_PARTICLE_REQUEST_RESULT_QUEUE_FULL;
    }

    queue->entries[queue->tail] = *request;
    queue->tail++;
    if (queue->tail >= queue->capacity) {
        queue->tail = 0u;
    }
    queue->count++;
    return GAME_PARTICLE_REQUEST_RESULT_OK;
}

GameParticleRequestResult game_particle_request_pop(GameParticleRequestQueue *queue, GameParticleRequest *out_request) {
    if (!game_particle_request_queue_is_valid(queue) || !out_request) {
        return GAME_PARTICLE_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    if (queue->count == 0u) {
        return GAME_PARTICLE_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    *out_request = queue->entries[queue->head];
    queue->head++;
    if (queue->head >= queue->capacity) {
        queue->head = 0u;
    }
    queue->count--;
    return GAME_PARTICLE_REQUEST_RESULT_OK;
}

GameParticleRequestResult game_particle_request_peek(const GameParticleRequestQueue *queue, GameParticleRequest *out_request) {
    if (!game_particle_request_queue_is_valid(queue) || !out_request) {
        return GAME_PARTICLE_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    if (queue->count == 0u) {
        return GAME_PARTICLE_REQUEST_RESULT_INVALID_ARGUMENT;
    }
    *out_request = queue->entries[queue->head];
    return GAME_PARTICLE_REQUEST_RESULT_OK;
}

GameParticleRequestResult game_particle_request_project_from_event(
    const GameEvent *event,
    uint64_t requested_tick,
    uint64_t source_event_sequence,
    GameParticleRequestQueue *queue
) {
    if (!event || !game_particle_request_queue_is_valid(queue)) {
        return GAME_PARTICLE_REQUEST_RESULT_INVALID_ARGUMENT;
    }

    GameParticleRequest request = {
        .requested_tick = requested_tick,
        .source_event_sequence = source_event_sequence,
        .intensity = 4u,
        .duration_ticks = 3u,
    };

    if (event->type == GAME_EVENT_TYPE_PROJECTILE_IMPACT) {
        if (event->payload_size < sizeof(GameProjectileImpactPayload)) {
            return GAME_PARTICLE_REQUEST_RESULT_BAD_PAYLOAD;
        }
        const GameProjectileImpactPayload *impact = (const GameProjectileImpactPayload *)event->payload.bytes;
        request.effect_id = 1u;
        request.origin_q = impact->target_q;
        request.origin_r = impact->target_r;
        request.intensity = (uint32_t)(impact->damage > 0 ? impact->damage : 1);
    } else if (event->type == GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED || event->type == GAME_EVENT_TYPE_STATUS_EFFECT_EXPIRED) {
        if (event->payload_size < sizeof(GameStatusEffectPayload)) {
            return GAME_PARTICLE_REQUEST_RESULT_BAD_PAYLOAD;
        }
        const GameStatusEffectPayload *status = (const GameStatusEffectPayload *)event->payload.bytes;
        request.effect_id = 2u;
        request.origin_q = status->target_q;
        request.origin_r = status->target_r;
        request.intensity = (uint32_t)(status->magnitude > 0 ? status->magnitude : 1u);
        request.duration_ticks = (uint32_t)(status->duration_ticks + 1u);
    } else {
        return GAME_PARTICLE_REQUEST_RESULT_UNKNOWN_EVENT;
    }

    return game_particle_request_push(queue, &request);
}
