#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_EVENT_MAX_PAYLOAD_BYTES 64

typedef enum {
    GAME_EVENT_TYPE_NULL = 0,
    GAME_EVENT_TYPE_SIMULATION = 1,
    GAME_EVENT_TYPE_DEBUG = 2,
    GAME_EVENT_TYPE_NOISE_EMITTED = 3,
    GAME_EVENT_TYPE_FIELD_IMPULSE_APPLIED = 4,
    GAME_EVENT_TYPE_ACTOR_ATTENTION_UPDATED = 5,
    GAME_EVENT_TYPE_TEST = 6,
    GAME_EVENT_TYPE_THREAT_STATE_CHANGED = 7,
    GAME_EVENT_TYPE_ABILITY_USED = 8,
    GAME_EVENT_TYPE_PROJECTILE_IMPACT = 9,
    GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED = 10,
    GAME_EVENT_TYPE_STATUS_EFFECT_EXPIRED = 11,
    GAME_EVENT_TYPE_PARTY_SELECTED = 12,
    GAME_EVENT_TYPE_HORDE_GROUP_UPDATED = 13,
    GAME_EVENT_TYPE_AUDIO_REQUEST = 14,
    GAME_EVENT_TYPE_PARTICLE_REQUEST = 15,
    GAME_EVENT_TYPE_PARTY_MOVED = 16,
    GAME_EVENT_TYPE_WORLD_INTERACTED = 17
} GameEventType;

typedef union {
    uint8_t bytes[GAME_EVENT_MAX_PAYLOAD_BYTES];
    uint32_t words[GAME_EVENT_MAX_PAYLOAD_BYTES / sizeof(uint32_t)];
    int32_t ints[GAME_EVENT_MAX_PAYLOAD_BYTES / sizeof(int32_t)];
    float floats[GAME_EVENT_MAX_PAYLOAD_BYTES / sizeof(float)];
} GameEventPayload;

typedef struct GameEvent {
    GameEventType type;
    uint64_t tick;
    GameEntityId source;
    uint8_t payload_size;
    GameEventPayload payload;
} GameEvent;

typedef enum {
    GAME_EVENT_QUEUE_RESULT_OK = 0,
    GAME_EVENT_QUEUE_RESULT_INVALID_ARGUMENT = 1,
    GAME_EVENT_QUEUE_RESULT_FULL = 2,
    GAME_EVENT_QUEUE_RESULT_EMPTY = 3,
    GAME_EVENT_QUEUE_RESULT_BAD_PAYLOAD = 4
} GameEventQueueResult;

typedef struct GameEventQueue {
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    GameEvent *events;
} GameEventQueue;

GameEventQueueResult game_event_queue_init(GameEventQueue *queue, size_t capacity);
void game_event_queue_destroy(GameEventQueue *queue);
GameEventQueueResult game_event_queue_push(GameEventQueue *queue, const GameEvent *event);
GameEventQueueResult game_event_queue_pop(GameEventQueue *queue, GameEvent *out_event);
GameEventQueueResult game_event_queue_peek(const GameEventQueue *queue, GameEvent *out_event);
GameEventQueueResult game_event_queue_clear(GameEventQueue *queue);
size_t game_event_queue_count(const GameEventQueue *queue);

#ifdef __cplusplus
}
#endif
