#include "sim/command.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

GameCommandQueueResult game_command_queue_init(GameCommandQueue *queue, const GameCommandQueueConfig *config) {
    if (!queue || !config || config->capacity == 0) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    memset(queue, 0, sizeof(*queue));

    queue->commands = (GameCommand *)malloc(config->capacity * sizeof(GameCommand));
    if (!queue->commands) {
        game_command_queue_destroy(queue);
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    queue->capacity = config->capacity;
    queue->accepted_tick_window = config->accepted_tick_window;
    return GAME_COMMAND_QUEUE_RESULT_OK;
}

void game_command_queue_destroy(GameCommandQueue *queue) {
    if (!queue) {
        return;
    }

    free(queue->commands);
    memset(queue, 0, sizeof(*queue));
}

static bool game_command_type_is_supported(GameCommandType type) {
    return type == GAME_COMMAND_TYPE_EMIT_NOISE || type == GAME_COMMAND_TYPE_MOVE_PARTY
           || type == GAME_COMMAND_TYPE_INTERACT_WORLD || type == GAME_COMMAND_TYPE_ATTACK_TARGET;
}

bool game_command_is_source_valid(const GameEntityRegistry *registry, GameEntityId source, bool require_alive) {
    if (!game_entity_id_is_valid(source)) {
        return false;
    }

    if (!registry) {
        return true;
    }

    return require_alive ? game_entity_registry_is_alive(registry, source) : source.index < game_entity_registry_capacity(registry);
}

GameCommandQueueResult game_command_queue_push(
    GameCommandQueue *queue,
    const GameCommand *command,
    uint64_t current_tick,
    const GameEntityRegistry *registry
) {
    if (!queue || !command) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (!queue->commands || queue->capacity == 0) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (!game_command_type_is_supported(command->type)) {
        return GAME_COMMAND_QUEUE_RESULT_UNSUPPORTED_COMMAND;
    }

    if (command->payload_size > GAME_COMMAND_MAX_PAYLOAD_BYTES) {
        return GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD;
    }

    if (!game_command_is_source_valid(registry, command->source, true)) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (command->requested_tick < current_tick) {
        return GAME_COMMAND_QUEUE_RESULT_STALE_TICK;
    }

    if (command->requested_tick > current_tick + queue->accepted_tick_window) {
        return GAME_COMMAND_QUEUE_RESULT_OUTSIDE_WINDOW;
    }

    if (queue->count == queue->capacity) {
        return GAME_COMMAND_QUEUE_RESULT_FULL;
    }

    queue->commands[queue->tail] = *command;
    queue->tail++;
    if (queue->tail == queue->capacity) {
        queue->tail = 0;
    }
    queue->count++;

    return GAME_COMMAND_QUEUE_RESULT_OK;
}

GameCommandQueueResult game_command_queue_pop(GameCommandQueue *queue, GameCommand *out_command) {
    if (!queue) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (!queue->commands || queue->capacity == 0 || queue->count == 0) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (out_command) {
        *out_command = queue->commands[queue->head];
    }

    queue->head++;
    if (queue->head == queue->capacity) {
        queue->head = 0;
    }
    queue->count--;
    return GAME_COMMAND_QUEUE_RESULT_OK;
}

GameCommandQueueResult game_command_queue_peek(const GameCommandQueue *queue, GameCommand *out_command) {
    if (!queue || !out_command) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    if (!queue->commands || queue->capacity == 0 || queue->count == 0) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    *out_command = queue->commands[queue->head];
    return GAME_COMMAND_QUEUE_RESULT_OK;
}

GameCommandQueueResult game_command_queue_clear(GameCommandQueue *queue) {
    if (!queue) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    return GAME_COMMAND_QUEUE_RESULT_OK;
}

size_t game_command_queue_count(const GameCommandQueue *queue) {
    return queue ? queue->count : 0u;
}
