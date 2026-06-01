#include "sim/sim_context.h"

#include <stdbool.h>

static GameSimContextResult game_sim_context_run_systems(GameSimContext *context) {
    (void)context;
    return GAME_SIM_CONTEXT_RESULT_OK;
}

GameSimContextResult game_sim_context_init(GameSimContext *context, const GameSimContextConfig *config) {
    if (!context || !config) {
        return GAME_SIM_CONTEXT_RESULT_INVALID_ARGUMENT;
    }

    if (context->initialized) {
        return GAME_SIM_CONTEXT_RESULT_ALREADY_INITIALIZED;
    }

    if (config->initial_entity_capacity == 0 || config->initial_event_capacity == 0) {
        return GAME_SIM_CONTEXT_RESULT_INVALID_ARGUMENT;
    }

    if (game_entity_registry_init(&context->entity_registry, config->initial_entity_capacity) != GAME_ENTITY_RESULT_OK) {
        return GAME_SIM_CONTEXT_RESULT_INIT_FAILURE;
    }

    if (game_event_queue_init(&context->event_queue, config->initial_event_capacity) != GAME_EVENT_QUEUE_RESULT_OK) {
        game_entity_registry_destroy(&context->entity_registry);
        return GAME_SIM_CONTEXT_RESULT_INIT_FAILURE;
    }

    context->initialized = true;
    context->tick = 0u;
    game_rng_seed(&context->rng, config->rng_seed);
    return GAME_SIM_CONTEXT_RESULT_OK;
}

void game_sim_context_shutdown(GameSimContext *context) {
    if (!context) {
        return;
    }

    game_event_queue_destroy(&context->event_queue);
    game_entity_registry_destroy(&context->entity_registry);
    context->initialized = false;
    context->tick = 0u;
    context->rng.state = 0u;
}

bool game_sim_context_is_initialized(const GameSimContext *context) {
    return context ? context->initialized : false;
}

GameSimContextResult game_sim_context_tick(GameSimContext *context) {
    if (!context || !context->initialized) {
        return GAME_SIM_CONTEXT_RESULT_INVALID_ARGUMENT;
    }

    context->tick++;
    return game_sim_context_run_systems(context);
}

uint64_t game_sim_context_tick_count(const GameSimContext *context) {
    return context ? context->tick : 0u;
}

uint64_t game_sim_context_rng_state(const GameSimContext *context) {
    return context ? context->rng.state : 0u;
}
