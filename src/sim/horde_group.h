#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "world/topology.h"
#include "sim/horde_attention.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t GameHordeGroupId;

typedef enum {
    GAME_HORDE_GROUP_RESULT_OK = 0,
    GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT = 1,
    GAME_HORDE_GROUP_RESULT_EVENT_LOG_FULL = 2,
} GameHordeGroupResult;

typedef struct {
    uint32_t pressure_gain_per_sample;
    uint32_t decay_per_tick;
    uint32_t threatened_threshold;
    uint32_t calm_threshold;
    uint32_t migration_threshold;
    uint32_t migration_transfer;
} GameHordeGroupConfig;

typedef struct {
    GameHordeGroupId id;
    uint32_t pressure;
    uint32_t mass_estimate;
    GameWorldTopologyRegionId region_id;
    GameEntityId interest_source;
    uint64_t last_update_tick;
    GameHordePosture posture;
} GameHordeGroupState;

typedef struct {
    uint32_t previous_pressure;
    uint32_t current_pressure;
    uint32_t previous_mass_estimate;
    uint32_t current_mass_estimate;
    GameHordePosture previous_posture;
    GameHordePosture current_posture;
    GameWorldTopologyRegionId previous_region;
    GameWorldTopologyRegionId current_region;
    uint32_t migrated_pressure;
    bool migrated;
    bool posture_changed;
    bool pressure_changed;
    bool region_changed;
    bool mass_changed;
    uint64_t emitted_event_sequence;
    bool emitted;
} GameHordeGroupUpdateResult;

GameHordeGroupResult game_horde_group_init(GameHordeGroupState *group, GameHordeGroupId group_id, GameWorldTopologyRegionId region_id);
GameHordeGroupResult game_horde_group_collect_neighbors(
    const GameWorldTopology *topology,
    GameWorldTopologyRegionId region_id,
    GameWorldTopologyRegionId *out_neighbors,
    size_t capacity,
    size_t *out_neighbor_count
);

GameHordeGroupResult game_horde_group_tick(
    GameHordeGroupState *group,
    const GameHordeGroupConfig *config,
    uint32_t field_sample,
    GameEntityId interest_source,
    const GameWorldTopologyRegionId *neighbor_regions,
    const uint32_t *neighbor_samples,
    size_t neighbor_count,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    GameHordeGroupUpdateResult *out_result
);

#ifdef __cplusplus
}
#endif
