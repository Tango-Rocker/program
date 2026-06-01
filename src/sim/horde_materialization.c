#include "sim/horde_materialization.h"

#include <stdbool.h>
#include <string.h>

#include "ecs/entity.h"

static GameHordeMaterializationResult game_horde_materialization_record_trace(
    const GameHordeMaterializationState *state,
    const GameHordeGroupState *group,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    uint64_t *out_event_seq
) {
    if (!event_log) {
        return GAME_HORDE_MATERIALIZATION_RESULT_OK;
    }

    if (game_event_log_append(
            event_log,
            tick,
            GAME_EVENT_TYPE_HORDE_GROUP_UPDATED,
            (GameEntityId){state->group_id, group->region_id},
            parent_event_sequence,
            out_event_seq
        ) != GAME_EVENT_LOG_RESULT_OK) {
        return GAME_HORDE_MATERIALIZATION_RESULT_EVENT_LOG_FULL;
    }

    return GAME_HORDE_MATERIALIZATION_RESULT_OK;
}

static GameHordeMaterializationResult game_horde_materialization_record_queue_payload(
    const GameHordeMaterializationState *state,
    GameHordeMaterializationAction action,
    uint32_t actor_count,
    const GameEntityId *actor_ids,
    GameWorldTopologyRegionId region_id,
    uint64_t tick,
    uint64_t source_event_sequence,
    GameEventQueue *event_queue
) {
    if (!event_queue || actor_count > GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS) {
        return GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT;
    }
    if (actor_count > 0u && actor_ids == NULL) {
        return GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT;
    }

    GameHordeMaterializationEventPayload payload = {
        .group_id = state ? state->group_id : 0u,
        .region_id = region_id,
        .action = action,
        .actor_count = actor_count,
        .parent_event_sequence = source_event_sequence,
    };

    for (uint32_t i = 0u; i < actor_count; ++i) {
        payload.actor_ids[i] = actor_ids[i];
    }

    GameEvent event = {
        .type = GAME_EVENT_TYPE_HORDE_GROUP_UPDATED,
        .tick = tick,
        .source = {state ? state->group_id : 0u, region_id},
        .payload_size = (uint8_t)sizeof(payload),
    };
    memcpy(&event.payload, &payload, sizeof(payload));

    if (game_event_queue_push(event_queue, &event) != GAME_EVENT_QUEUE_RESULT_OK) {
        return GAME_HORDE_MATERIALIZATION_RESULT_EVENT_QUEUE_FULL;
    }
    return GAME_HORDE_MATERIALIZATION_RESULT_OK;
}

static uint32_t game_horde_materialization_compute_spawn_count(
    const GameHordeGroupState *group,
    const GameHordeMaterializationConfig *config,
    uint32_t available_slots
) {
    if (!group || !config || available_slots == 0u) {
        return 0u;
    }

    if (config->pressure_cost_per_actor == 0u || config->spawn_budget_per_tick == 0u) {
        return 0u;
    }
    if (group->pressure <= config->materialize_pressure_threshold) {
        return 0u;
    }

    uint32_t by_pressure = (group->pressure - config->materialize_pressure_threshold) / config->pressure_cost_per_actor;
    if (by_pressure == 0u) {
        return 0u;
    }

    uint32_t max_count = config->spawn_budget_per_tick;
    if (config->spawn_actor_count > 0u && config->spawn_actor_count < max_count) {
        max_count = config->spawn_actor_count;
    }
    if (available_slots < max_count) {
        max_count = available_slots;
    }

    return by_pressure < max_count ? by_pressure : max_count;
}

static GameHordeMaterializationResult game_horde_materialization_spawn(
    GameHordeMaterializationState *state,
    GameHordeGroupState *group,
    const GameHordeMaterializationConfig *config,
    uint64_t tick,
    uint64_t source_event_sequence,
    GameEntityRegistry *registry,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    GameHordeMaterializationUpdateResult *out_result
) {
    if (!state || !group || !config || !registry) {
        return GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT;
    }
    if (state->actor_count > state->actor_capacity) {
        return GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT;
    }

    uint32_t available_slots = (uint32_t)(state->actor_capacity - state->actor_count);
    uint32_t spawn_count = game_horde_materialization_compute_spawn_count(group, config, available_slots);
    if (spawn_count == 0u) {
        return GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE;
    }

    GameEntityId emitted_ids[GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS] = {0};
    uint32_t emitted = 0u;
    for (uint32_t i = 0u; i < spawn_count; ++i) {
        GameEntityId actor_id = game_entity_invalid_id();
        if (game_entity_registry_create(registry, &actor_id) != GAME_ENTITY_RESULT_OK) {
            break;
        }

        state->actor_ids[state->actor_count++] = actor_id;
        group->pressure -= config->pressure_cost_per_actor;
        if (UINT32_MAX - group->mass_estimate < config->mass_per_actor) {
            group->mass_estimate = UINT32_MAX;
        } else {
            group->mass_estimate += config->mass_per_actor;
        }

        ++out_result->spawned_count;
        if (emitted < GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS) {
            emitted_ids[emitted++] = actor_id;
        }
    }

    if (emitted == 0u) {
        return GAME_HORDE_MATERIALIZATION_RESULT_ENTITY_POOL_FULL;
    }
    if (out_result->spawned_count > 0u) {
        out_result->spawned = true;
        out_result->actor_count = out_result->spawned_count;
    }

    if (event_queue) {
        GameHordeMaterializationResult queue_result = game_horde_materialization_record_queue_payload(
            state,
            GAME_HORDE_MATERIALIZATION_ACTION_SPAWN,
            (emitted > GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS ? GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS : emitted),
            emitted_ids,
            group->region_id,
            tick,
            source_event_sequence,
            event_queue
        );
        if (queue_result != GAME_HORDE_MATERIALIZATION_RESULT_OK) {
            return queue_result;
        }
    }

    return game_horde_materialization_record_trace(
        state,
        group,
        tick,
        source_event_sequence,
        event_log,
        &out_result->emitted_event_sequence
    );
}

static GameHordeMaterializationResult game_horde_materialization_dematerialize(
    GameHordeMaterializationState *state,
    GameHordeGroupState *group,
    const GameHordeMaterializationConfig *config,
    uint64_t tick,
    uint64_t source_event_sequence,
    GameEntityRegistry *registry,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    GameHordeMaterializationUpdateResult *out_result
) {
    if (!state || !group || !config || !registry) {
        return GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT;
    }
    if (state->actor_count == 0u || state->actor_capacity == 0u) {
        return GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE;
    }
    if (group->pressure > config->dematerialize_pressure_threshold) {
        return GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE;
    }
    if (config->dematerialize_budget_per_tick == 0u) {
        return GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE;
    }

    uint32_t dematerialize_count = state->actor_count < config->dematerialize_budget_per_tick ? (uint32_t)state->actor_count
                                                                                    : config->dematerialize_budget_per_tick;
    if (dematerialize_count == 0u) {
        return GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE;
    }

    GameEntityId emitted_ids[GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS] = {0};
    uint32_t emitted = 0u;
    for (uint32_t i = 0u; i < dematerialize_count; ++i) {
        GameEntityId actor_id = state->actor_ids[--state->actor_count];
        (void)game_entity_registry_destroy_id(registry, actor_id);

        if (config->pressure_return_per_actor > 0u) {
            if (UINT32_MAX - group->pressure < config->pressure_return_per_actor) {
                group->pressure = UINT32_MAX;
            } else {
                group->pressure += config->pressure_return_per_actor;
            }
        }
        if (group->mass_estimate >= config->mass_per_actor) {
            group->mass_estimate -= config->mass_per_actor;
        } else {
            group->mass_estimate = 0u;
        }

        ++out_result->dematerialized_count;
        if (emitted < GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS) {
            emitted_ids[emitted++] = actor_id;
        }
    }

    if (out_result->dematerialized_count > 0u) {
        out_result->dematerialized = true;
        out_result->actor_count = out_result->dematerialized_count;
    }

    if (event_queue) {
        GameHordeMaterializationResult queue_result = game_horde_materialization_record_queue_payload(
            state,
            GAME_HORDE_MATERIALIZATION_ACTION_DEMATERIALIZE,
            (emitted > GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS ? GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS : emitted),
            emitted_ids,
            group->region_id,
            tick,
            source_event_sequence,
            event_queue
        );
        if (queue_result != GAME_HORDE_MATERIALIZATION_RESULT_OK) {
            return queue_result;
        }
    }

    return game_horde_materialization_record_trace(
        state,
        group,
        tick,
        source_event_sequence,
        event_log,
        &out_result->emitted_event_sequence
    );
}

void game_horde_materialization_init(
    GameHordeMaterializationState *state,
    GameHordeGroupId group_id,
    GameEntityId *actor_ids,
    size_t actor_capacity
) {
    if (!state || !actor_ids || actor_capacity == 0u) {
        return;
    }

    *state = (GameHordeMaterializationState){
        .actor_capacity = actor_capacity,
        .actor_count = 0u,
        .actor_ids = actor_ids,
        .group_id = group_id,
    };
}

GameHordeMaterializationResult game_horde_materialization_apply(
    GameHordeMaterializationState *state,
    GameHordeGroupState *group,
    const GameHordeMaterializationConfig *config,
    bool region_visible,
    uint32_t proximity_metric,
    uint64_t tick,
    uint64_t source_event_sequence,
    GameEntityRegistry *entity_registry,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    GameHordeMaterializationUpdateResult *out_result
) {
    if (!state || !group || !config || !entity_registry || !out_result) {
        return GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT;
    }
    if (!state->actor_ids || state->actor_capacity == 0u) {
        return GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT;
    }
    if (state->actor_count > state->actor_capacity) {
        return GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT;
    }

    *out_result = (GameHordeMaterializationUpdateResult){0};
    out_result->pressure_before = group->pressure;

    bool in_spawn_range = (config->max_spawn_proximity == 0u || proximity_metric <= config->max_spawn_proximity);
    GameHordeMaterializationResult result = GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE;
    if (region_visible && in_spawn_range && group->posture != GAME_HORDE_POSTURE_DORMANT) {
        result = game_horde_materialization_spawn(
            state,
            group,
            config,
            tick,
            source_event_sequence,
            entity_registry,
            event_queue,
            event_log,
            out_result
        );
    }

    if (
        result == GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE || result == GAME_HORDE_MATERIALIZATION_RESULT_ENTITY_POOL_FULL
    ) {
        result = game_horde_materialization_dematerialize(
            state,
            group,
            config,
            tick,
            source_event_sequence,
            entity_registry,
            event_queue,
            event_log,
            out_result
        );
    }

    out_result->pressure_after = group->pressure;
    return result;
}
