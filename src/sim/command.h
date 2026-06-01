#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_COMMAND_MAX_PAYLOAD_BYTES GAME_EVENT_MAX_PAYLOAD_BYTES

typedef enum {
    GAME_COMMAND_TYPE_NULL = 0,
    GAME_COMMAND_TYPE_EMIT_NOISE = 1,
    GAME_COMMAND_TYPE_MOVE_PARTY = 2,
    GAME_COMMAND_TYPE_INTERACT_WORLD = 3,
    GAME_COMMAND_TYPE_ATTACK_TARGET = 4,
} GameCommandType;

typedef union {
    uint8_t bytes[GAME_COMMAND_MAX_PAYLOAD_BYTES];
    uint32_t words[GAME_COMMAND_MAX_PAYLOAD_BYTES / sizeof(uint32_t)];
    int32_t ints[GAME_COMMAND_MAX_PAYLOAD_BYTES / sizeof(int32_t)];
} GameCommandPayload;

typedef struct GameCommand {
    GameCommandType type;
    uint64_t requested_tick;
    GameEntityId source;
    uint8_t payload_size;
    GameCommandPayload payload;
} GameCommand;

typedef struct GameCommandNoisePayload {
    int32_t origin_q;
    int32_t origin_r;
    int32_t intensity;
    uint16_t max_radius;
    uint16_t attenuation;
    uint16_t decay_ticks;
    uint16_t reserved;
} GameCommandNoisePayload;

typedef struct GameCommandTilePayload {
    int32_t target_q;
    int32_t target_r;
    uint32_t flags;
} GameCommandTilePayload;

typedef enum {
    GAME_COMMAND_QUEUE_RESULT_OK = 0,
    GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT = 1,
    GAME_COMMAND_QUEUE_RESULT_UNSUPPORTED_COMMAND = 2,
    GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD = 3,
    GAME_COMMAND_QUEUE_RESULT_STALE_TICK = 4,
    GAME_COMMAND_QUEUE_RESULT_OUTSIDE_WINDOW = 5,
    GAME_COMMAND_QUEUE_RESULT_FULL = 6
} GameCommandQueueResult;

typedef struct GameCommandQueueConfig {
    size_t capacity;
    uint32_t accepted_tick_window;
} GameCommandQueueConfig;

typedef struct GameCommandQueue {
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    uint32_t accepted_tick_window;
    GameCommand *commands;
} GameCommandQueue;

GameCommandQueueResult game_command_queue_init(GameCommandQueue *queue, const GameCommandQueueConfig *config);
void game_command_queue_destroy(GameCommandQueue *queue);
GameCommandQueueResult game_command_queue_push(
    GameCommandQueue *queue,
    const GameCommand *command,
    uint64_t current_tick,
    const GameEntityRegistry *registry
);
GameCommandQueueResult game_command_queue_pop(GameCommandQueue *queue, GameCommand *out_command);
GameCommandQueueResult game_command_queue_peek(const GameCommandQueue *queue, GameCommand *out_command);
GameCommandQueueResult game_command_queue_clear(GameCommandQueue *queue);
size_t game_command_queue_count(const GameCommandQueue *queue);
bool game_command_is_source_valid(const GameEntityRegistry *registry, GameEntityId source, bool require_alive);

#ifdef __cplusplus
}
#endif
