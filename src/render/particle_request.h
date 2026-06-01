#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_PARTICLE_REQUEST_RESULT_OK = 0,
    GAME_PARTICLE_REQUEST_RESULT_INVALID_ARGUMENT = 1,
    GAME_PARTICLE_REQUEST_RESULT_UNKNOWN_EVENT = 2,
    GAME_PARTICLE_REQUEST_RESULT_QUEUE_FULL = 3,
    GAME_PARTICLE_REQUEST_RESULT_BAD_PAYLOAD = 4,
} GameParticleRequestResult;

typedef struct {
    uint32_t effect_id;
    int32_t origin_q;
    int32_t origin_r;
    uint64_t source_event_sequence;
    uint64_t requested_tick;
    uint32_t intensity;
    uint32_t duration_ticks;
} GameParticleRequest;

typedef struct {
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    GameParticleRequest *entries;
} GameParticleRequestQueue;

GameParticleRequestResult game_particle_request_init(GameParticleRequestQueue *queue, GameParticleRequest *entries, size_t capacity);
void game_particle_request_destroy(GameParticleRequestQueue *queue);
void game_particle_request_clear(GameParticleRequestQueue *queue);
GameParticleRequestResult game_particle_request_push(GameParticleRequestQueue *queue, const GameParticleRequest *request);
GameParticleRequestResult game_particle_request_pop(GameParticleRequestQueue *queue, GameParticleRequest *out_request);
GameParticleRequestResult game_particle_request_peek(const GameParticleRequestQueue *queue, GameParticleRequest *out_request);
size_t game_particle_request_count(const GameParticleRequestQueue *queue);

GameParticleRequestResult game_particle_request_project_from_event(
    const GameEvent *event,
    uint64_t requested_tick,
    uint64_t source_event_sequence,
    GameParticleRequestQueue *queue
);

#ifdef __cplusplus
}
#endif
