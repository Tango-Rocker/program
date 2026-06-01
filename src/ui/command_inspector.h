#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "sim/command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool has_staged;
    GameCommand staged;
    GameCommandQueueResult last_result;
} GameCommandInspector;

void game_command_inspector_init(GameCommandInspector *inspector);
GameCommandQueueResult game_command_inspector_stage_noise(
    GameCommandInspector *inspector,
    GameEntityId source,
    uint64_t requested_tick,
    const GameCommandNoisePayload *payload
);
GameCommandQueueResult game_command_inspector_validate(
    const GameCommandInspector *inspector,
    uint64_t current_tick,
    const GameEntityRegistry *registry
);
GameCommandQueueResult game_command_inspector_submit(
    GameCommandInspector *inspector,
    GameCommandQueue *queue,
    uint64_t current_tick,
    const GameEntityRegistry *registry
);
const char *game_command_inspector_result_message(GameCommandQueueResult result);

#ifdef __cplusplus
}
#endif
