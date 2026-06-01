#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "core/random.h"
#include "ecs/entity.h"
#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GameSimContextConfig {
    uint64_t rng_seed;
    size_t initial_entity_capacity;
    size_t initial_event_capacity;
} GameSimContextConfig;

typedef enum {
    GAME_SIM_CONTEXT_RESULT_OK = 0,
    GAME_SIM_CONTEXT_RESULT_INVALID_ARGUMENT = 1,
    GAME_SIM_CONTEXT_RESULT_NOT_INITIALIZED = 2,
    GAME_SIM_CONTEXT_RESULT_ALREADY_INITIALIZED = 3,
    GAME_SIM_CONTEXT_RESULT_INIT_FAILURE = 4,
} GameSimContextResult;

typedef struct GameSimContext {
    bool initialized;
    uint64_t tick;
    GameRng rng;
    GameEntityRegistry entity_registry;
    GameEventQueue event_queue;
} GameSimContext;

GameSimContextResult game_sim_context_init(GameSimContext *context, const GameSimContextConfig *config);
void game_sim_context_shutdown(GameSimContext *context);
bool game_sim_context_is_initialized(const GameSimContext *context);
GameSimContextResult game_sim_context_tick(GameSimContext *context);
uint64_t game_sim_context_tick_count(const GameSimContext *context);
uint64_t game_sim_context_rng_state(const GameSimContext *context);

#ifdef __cplusplus
}
#endif
