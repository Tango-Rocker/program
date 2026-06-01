#include "sim/horde_group.h"

#include <string.h>

static GameHordePosture game_horde_group_next_posture(const GameHordeGroupConfig *config, uint32_t pressure) {
    if (pressure >= config->threatened_threshold) {
        return GAME_HORDE_POSTURE_THREATENED;
    }
    if (pressure <= config->calm_threshold) {
        return GAME_HORDE_POSTURE_DORMANT;
    }
    return GAME_HORDE_POSTURE_ALERT;
}

GameHordeGroupResult game_horde_group_init(
    GameHordeGroupState *group,
    GameHordeGroupId group_id,
    GameWorldTopologyRegionId region_id
) {
    if (!group) {
        return GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT;
    }
    *group = (GameHordeGroupState){
        .id = group_id,
        .pressure = 0u,
        .mass_estimate = 0u,
        .region_id = region_id,
        .interest_source = {UINT32_MAX, UINT32_MAX},
        .last_update_tick = 0u,
        .posture = GAME_HORDE_POSTURE_DORMANT,
    };
    return GAME_HORDE_GROUP_RESULT_OK;
}

GameHordeGroupResult game_horde_group_collect_neighbors(
    const GameWorldTopology *topology,
    GameWorldTopologyRegionId region_id,
    GameWorldTopologyRegionId *out_neighbors,
    size_t capacity,
    size_t *out_neighbor_count
) {
    if (!topology || !out_neighbor_count) {
        return GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT;
    }

    *out_neighbor_count = 0u;
    if (!out_neighbors && capacity > 0u) {
        return GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT;
    }

    if (!topology->portals) {
        return GAME_HORDE_GROUP_RESULT_OK;
    }

    for (size_t i = 0u; i < topology->portal_count; ++i) {
        GameWorldTopologyPortal portal = topology->portals[i];
        GameWorldTopologyRegionId neighbor = UINT32_MAX;
        if (portal.region_a == region_id) {
            neighbor = portal.region_b;
        } else if (portal.region_b == region_id) {
            neighbor = portal.region_a;
        }

        if (neighbor == UINT32_MAX) {
            continue;
        }

        bool duplicate = false;
        for (size_t existing = 0u; existing < *out_neighbor_count; ++existing) {
            if (out_neighbors[existing] == neighbor) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }

        if (*out_neighbor_count >= capacity) {
            return GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT;
        }
        out_neighbors[*out_neighbor_count] = neighbor;
        ++(*out_neighbor_count);
    }

    for (size_t i = 1u; i < *out_neighbor_count; ++i) {
        GameWorldTopologyRegionId key = out_neighbors[i];
        size_t j = i;
        while (j > 0u && out_neighbors[j - 1u] > key) {
            out_neighbors[j] = out_neighbors[j - 1u];
            --j;
        }
        out_neighbors[j] = key;
    }

    return GAME_HORDE_GROUP_RESULT_OK;
}

static GameHordeGroupResult game_horde_group_append_trace(
    const GameHordeGroupState *group,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventLog *event_log,
    uint64_t *out_sequence
) {
    if (!event_log) {
        return GAME_HORDE_GROUP_RESULT_OK;
    }

    if (game_event_log_append(
            event_log,
            tick,
            GAME_EVENT_TYPE_HORDE_GROUP_UPDATED,
            (GameEntityId){group->id, group->region_id},
            parent_event_sequence,
            out_sequence
        ) != GAME_EVENT_LOG_RESULT_OK) {
        return GAME_HORDE_GROUP_RESULT_EVENT_LOG_FULL;
    }
    return GAME_HORDE_GROUP_RESULT_OK;
}

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
) {
    if (!group || !config || !out_result) {
        return GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT;
    }
    if (neighbor_count > 0u && (!neighbor_regions || !neighbor_samples)) {
        return GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT;
    }
    if (config->decay_per_tick == 0u && config->pressure_gain_per_sample == 0u) {
        return GAME_HORDE_GROUP_RESULT_INVALID_ARGUMENT;
    }

    GameHordeGroupUpdateResult result = {
        .previous_pressure = group->pressure,
        .current_pressure = group->pressure,
        .previous_mass_estimate = group->mass_estimate,
        .current_mass_estimate = group->mass_estimate,
        .previous_posture = group->posture,
        .current_posture = group->posture,
        .previous_region = group->region_id,
        .current_region = group->region_id,
    };

    uint64_t elapsed = (group->last_update_tick == 0u) ? 0u : (tick - group->last_update_tick);
    group->last_update_tick = tick;

    for (uint64_t i = 0u; i < elapsed; ++i) {
        if (group->pressure == 0u) {
            continue;
        }
        if (config->decay_per_tick >= group->pressure) {
            group->pressure = 0u;
        } else {
            group->pressure -= config->decay_per_tick;
        }
    }

    if (field_sample > 0u) {
        uint64_t gain64 = (uint64_t)config->pressure_gain_per_sample * (uint64_t)field_sample;
        uint32_t gain = gain64 > UINT32_MAX ? UINT32_MAX : (uint32_t)gain64;
        if (UINT32_MAX - group->pressure < gain) {
            group->pressure = UINT32_MAX;
        } else {
            group->pressure += gain;
        }
        group->interest_source = interest_source;
    }

    result.current_pressure = group->pressure;
    if (result.current_pressure != result.previous_pressure) {
        result.pressure_changed = true;
    }

    result.current_posture = game_horde_group_next_posture(config, result.current_pressure);
    if (result.current_posture != result.previous_posture) {
        result.posture_changed = true;
    }

    if (group->pressure >= config->migration_threshold && neighbor_count > 0u && config->migration_transfer > 0u) {
        GameWorldTopologyRegionId best_region = UINT32_MAX;
        uint32_t best_sample = 0u;

        for (size_t i = 0u; i < neighbor_count; ++i) {
            GameWorldTopologyRegionId region = neighbor_regions[i];
            uint32_t sample = neighbor_samples ? neighbor_samples[i] : 0u;
            if (sample > best_sample || (sample == best_sample && sample > 0u && region < best_region)) {
                best_sample = sample;
                best_region = region;
            }
        }

        if (best_sample > 0u) {
            uint32_t transfer = config->migration_transfer < group->pressure ? config->migration_transfer : group->pressure;
            group->pressure -= transfer;
            result.migrated = true;
            result.migrated_pressure = transfer;
            result.current_region = best_region;
            result.region_changed = true;
            group->region_id = best_region;
            if (group->mass_estimate > 0u) {
                result.current_mass_estimate = group->mass_estimate;
            }
            result.current_pressure = group->pressure;
            if (result.current_pressure != result.previous_pressure) {
                result.pressure_changed = true;
            }
        }
    }

    result.current_posture = game_horde_group_next_posture(config, result.current_pressure);
    group->posture = result.current_posture;
    if (result.current_posture != result.previous_posture) {
        result.posture_changed = true;
    }

    if (result.region_changed) {
        result.current_region = group->region_id;
    } else {
        result.current_region = group->region_id;
    }

    if (result.pressure_changed || result.posture_changed || result.region_changed) {
        if (game_horde_group_append_trace(group, tick, parent_event_sequence, event_log, &result.emitted_event_sequence)
            != GAME_HORDE_GROUP_RESULT_OK) {
            *out_result = result;
            return GAME_HORDE_GROUP_RESULT_EVENT_LOG_FULL;
        }
        result.emitted = true;
    }

    *out_result = result;
    return GAME_HORDE_GROUP_RESULT_OK;
}
