#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_AUDIO_REQUEST_RESULT_OK = 0,
    GAME_AUDIO_REQUEST_RESULT_INVALID_ARGUMENT = 1,
    GAME_AUDIO_REQUEST_RESULT_UNKNOWN_EVENT = 2,
    GAME_AUDIO_REQUEST_RESULT_QUEUE_FULL = 3,
    GAME_AUDIO_REQUEST_RESULT_BAD_PAYLOAD = 4
} GameAudioRequestResult;

typedef struct {
    uint32_t sound_id;
    int32_t origin_q;
    int32_t origin_r;
    uint32_t priority;
    uint64_t source_event_sequence;
    uint64_t requested_tick;
} GameAudioRequest;

typedef struct {
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    GameAudioRequest *entries;
} GameAudioRequestQueue;

GameAudioRequestResult game_audio_request_init(GameAudioRequestQueue *queue, GameAudioRequest *entries, size_t capacity);
void game_audio_request_destroy(GameAudioRequestQueue *queue);
void game_audio_request_clear(GameAudioRequestQueue *queue);
GameAudioRequestResult game_audio_request_push(GameAudioRequestQueue *queue, const GameAudioRequest *request);
GameAudioRequestResult game_audio_request_pop(GameAudioRequestQueue *queue, GameAudioRequest *out_request);
GameAudioRequestResult game_audio_request_peek(const GameAudioRequestQueue *queue, GameAudioRequest *out_request);
size_t game_audio_request_count(const GameAudioRequestQueue *queue);

GameAudioRequestResult game_audio_request_project_from_event(
    const GameEvent *event,
    uint64_t requested_tick,
    uint64_t source_event_sequence,
    GameAudioRequestQueue *queue
);

#ifdef __cplusplus
}
#endif
