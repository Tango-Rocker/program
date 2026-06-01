#include "ui/command_inspector.h"

#include <string.h>

void game_command_inspector_init(GameCommandInspector *inspector) {
    if (!inspector) {
        return;
    }
    *inspector = (GameCommandInspector){
        .has_staged = false,
        .staged = {0},
        .last_result = GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT,
    };
}

GameCommandQueueResult game_command_inspector_stage_noise(
    GameCommandInspector *inspector,
    GameEntityId source,
    uint64_t requested_tick,
    const GameCommandNoisePayload *payload
) {
    if (!inspector || !payload) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    inspector->staged = (GameCommand){
        .type = GAME_COMMAND_TYPE_EMIT_NOISE,
        .requested_tick = requested_tick,
        .source = source,
        .payload_size = sizeof(*payload),
    };
    memcpy(inspector->staged.payload.bytes, payload, sizeof(*payload));
    inspector->has_staged = true;
    inspector->last_result = GAME_COMMAND_QUEUE_RESULT_OK;
    return GAME_COMMAND_QUEUE_RESULT_OK;
}

GameCommandQueueResult game_command_inspector_validate(
    const GameCommandInspector *inspector,
    uint64_t current_tick,
    const GameEntityRegistry *registry
) {
    if (!inspector || !inspector->has_staged) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }
    if (inspector->staged.type != GAME_COMMAND_TYPE_EMIT_NOISE) {
        return GAME_COMMAND_QUEUE_RESULT_UNSUPPORTED_COMMAND;
    }
    if (inspector->staged.payload_size != sizeof(GameCommandNoisePayload)) {
        return GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD;
    }
    if (!game_command_is_source_valid(registry, inspector->staged.source, true)) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }
    if (inspector->staged.requested_tick < current_tick) {
        return GAME_COMMAND_QUEUE_RESULT_STALE_TICK;
    }
    return GAME_COMMAND_QUEUE_RESULT_OK;
}

GameCommandQueueResult game_command_inspector_submit(
    GameCommandInspector *inspector,
    GameCommandQueue *queue,
    uint64_t current_tick,
    const GameEntityRegistry *registry
) {
    if (!inspector || !queue || !inspector->has_staged) {
        return GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT;
    }

    GameCommandQueueResult result = game_command_inspector_validate(inspector, current_tick, registry);
    if (result == GAME_COMMAND_QUEUE_RESULT_OK) {
        result = game_command_queue_push(queue, &inspector->staged, current_tick, registry);
    }

    inspector->last_result = result;
    if (result == GAME_COMMAND_QUEUE_RESULT_OK) {
        inspector->has_staged = false;
    }
    return result;
}

const char *game_command_inspector_result_message(GameCommandQueueResult result) {
    switch (result) {
    case GAME_COMMAND_QUEUE_RESULT_OK:
        return "ok";
    case GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT:
        return "invalid argument";
    case GAME_COMMAND_QUEUE_RESULT_UNSUPPORTED_COMMAND:
        return "unsupported command";
    case GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD:
        return "bad payload";
    case GAME_COMMAND_QUEUE_RESULT_STALE_TICK:
        return "stale tick";
    case GAME_COMMAND_QUEUE_RESULT_OUTSIDE_WINDOW:
        return "outside accepted tick window";
    case GAME_COMMAND_QUEUE_RESULT_FULL:
        return "command queue full";
    default:
        return "unknown command result";
    }
}
