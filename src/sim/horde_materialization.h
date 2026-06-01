#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "event/event_queue.h"
#include "sim/horde_group.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS 4u

typedef enum {
    GAME_HORDE_MATERIALIZATION_RESULT_OK = 0,
    GAME_HORDE_MATERIALIZATION_RESULT_INVALID_ARGUMENT = 1,
    GAME_HORDE_MATERIALIZATION_RESULT_ENTITY_POOL_FULL = 2,
    GAME_HORDE_MATERIALIZATION_RESULT_EVENT_QUEUE_FULL = 3,
    GAME_HORDE_MATERIALIZATION_RESULT_EVENT_LOG_FULL = 4,
    GAME_HORDE_MATERIALIZATION_RESULT_NO_RESOURCE = 5,
} GameHordeMaterializationResult;

typedef struct {
    uint32_t materialize_pressure_threshold;
    uint32_t dematerialize_pressure_threshold;
    uint32_t pressure_cost_per_actor;
    uint32_t pressure_return_per_actor;
    uint32_t spawn_budget_per_tick;
    uint32_t dematerialize_budget_per_tick;
    uint32_t mass_per_actor;
    uint32_t max_spawn_proximity;
    uint32_t spawn_actor_count;
} GameHordeMaterializationConfig;

typedef struct {
    size_t actor_capacity;
    size_t actor_count;
    GameEntityId *actor_ids;
    GameHordeGroupId group_id;
} GameHordeMaterializationState;

typedef struct {
    GameEntityId actor_ids[GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS];
    uint32_t actor_count;
    uint32_t spawned_count;
    uint32_t dematerialized_count;
    uint32_t pressure_before;
    uint32_t pressure_after;
    uint64_t emitted_event_sequence;
    bool dematerialized;
    bool spawned;
} GameHordeMaterializationUpdateResult;

typedef enum {
    GAME_HORDE_MATERIALIZATION_ACTION_NONE = 0u,
    GAME_HORDE_MATERIALIZATION_ACTION_SPAWN = 1u,
    GAME_HORDE_MATERIALIZATION_ACTION_DEMATERIALIZE = 2u,
} GameHordeMaterializationAction;

typedef struct {
    uint32_t group_id;
    GameWorldTopologyRegionId region_id;
    GameHordeMaterializationAction action;
    uint32_t actor_count;
    GameEntityId actor_ids[GAME_HORDE_MATERIALIZATION_MAX_EVENT_ACTORS];
    uint64_t parent_event_sequence;
} GameHordeMaterializationEventPayload;

void game_horde_materialization_init(
    GameHordeMaterializationState *state,
    GameHordeGroupId group_id,
    GameEntityId *actor_ids,
    size_t actor_capacity
);

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
);

#ifdef __cplusplus
}
#endif
