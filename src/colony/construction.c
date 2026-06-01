#include "colony/construction.h"

#include <stdint.h>
#include <string.h>
#include "colony/worker_ai.h"

static bool game_construction_handle_matches(const GameConstructionWorksite *site, GameConstructionWorksiteHandle handle) {
    return site->in_use && site->handle.slot == handle.slot && site->handle.version == handle.version;
}

static GameConstructionWorksite *game_construction_lookup_slot(
    GameConstructionSystem *system,
    GameConstructionWorksiteHandle handle
) {
    if (!system || !system->sites || handle.slot >= system->capacity) {
        return NULL;
    }

    GameConstructionWorksite *site = &system->sites[handle.slot];
    if (!game_construction_handle_matches(site, handle)) {
        return NULL;
    }

    return site;
}

static const GameConstructionWorksite *game_construction_lookup_slot_const(
    const GameConstructionSystem *system,
    GameConstructionWorksiteHandle handle
) {
    if (!system || !system->sites || handle.slot >= system->capacity) {
        return NULL;
    }

    const GameConstructionWorksite *site = &system->sites[handle.slot];
    if (!game_construction_handle_matches(site, handle)) {
        return NULL;
    }

    return site;
}

static uint32_t game_construction_next_version(uint32_t version) {
    return version == UINT32_MAX ? 1u : (version + 1u);
}

static bool game_construction_record_audit(
    GameConstructionAuditLog *audit_log,
    GameConstructionWorksiteHandle handle,
    uint64_t tick,
    GameConstructionWorksiteState state,
    GameConstructionAuditReason reason,
    uint32_t requirement_index,
    uint32_t requirement_delivered,
    uint32_t progress_ticks
) {
    if (!audit_log || !audit_log->entries || audit_log->count >= audit_log->capacity) {
        return false;
    }

    audit_log->entries[audit_log->count++] = (GameConstructionAuditEntry){
        .worksite = handle,
        .tick = tick,
        .state = state,
        .reason = reason,
        .requirement_index = requirement_index,
        .requirement_delivered = requirement_delivered,
        .progress_ticks = progress_ticks,
    };
    return true;
}

void game_construction_audit_init(GameConstructionAuditLog *log, GameConstructionAuditEntry *entries, size_t capacity) {
    if (!log) {
        return;
    }

    log->entries = entries;
    log->capacity = entries ? capacity : 0u;
    log->count = 0u;
}

GameConstructionResult game_construction_init(
    GameConstructionSystem *system,
    GameConstructionWorksite *sites,
    size_t capacity,
    uint32_t next_stable_id
) {
    if (!system || !sites || capacity == 0u || next_stable_id == 0u) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    system->sites = sites;
    system->capacity = capacity;
    system->next_stable_id = next_stable_id;

    for (size_t i = 0u; i < capacity; ++i) {
        sites[i] = (GameConstructionWorksite){
            .handle = {(uint32_t)i, 1u},
            .stable_id = 0u,
            .in_use = false,
            .state = GAME_CONSTRUCTION_STATE_PLANNING,
            .resource_source_owner = {0},
            .resource_destination_owner = {0},
            .required_work_ticks = 1u,
            .progress_ticks = 0u,
            .active_requirement_index = UINT32_MAX,
            .worker_capacity = 1u,
            .haul_route_request = {0u, 0u},
            .work_order = {0u, 0u},
            .active_haul = {0u, 0u},
            .structure_placement = {0u, 0u},
            .has_structure_placement = false,
            .requirement_count = 0u,
        };
    }

    return GAME_CONSTRUCTION_RESULT_OK;
}

static GameConstructionResult game_construction_map_job_board(GameJobBoardResult result) {
    switch (result) {
        case GAME_JOB_BOARD_RESULT_OK:
            return GAME_CONSTRUCTION_RESULT_OK;
        case GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT:
        case GAME_JOB_BOARD_RESULT_INVALID_HANDLE:
        case GAME_JOB_BOARD_RESULT_NO_SLOT:
        case GAME_JOB_BOARD_RESULT_ALREADY_RESERVED:
        case GAME_JOB_BOARD_RESULT_INVALID_STATE:
            return GAME_CONSTRUCTION_RESULT_JOB_BOARD_ERROR;
        default:
            return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }
}

static GameConstructionResult game_construction_map_inventory(GameInventoryResult result) {
    switch (result) {
        case GAME_INVENTORY_RESULT_OK:
            return GAME_CONSTRUCTION_RESULT_OK;
        case GAME_INVENTORY_RESULT_INVALID_ARGUMENT:
        case GAME_INVENTORY_RESULT_INVALID_HANDLE:
        case GAME_INVENTORY_RESULT_INVALID_OWNER:
        case GAME_INVENTORY_RESULT_ENTRY_NOT_FOUND:
        case GAME_INVENTORY_RESULT_RESERVATION_STALE:
        case GAME_INVENTORY_RESULT_RESERVATION_FULL:
        case GAME_INVENTORY_RESULT_RESERVATION_NOT_FOUND:
            return GAME_CONSTRUCTION_RESULT_INVENTORY_ERROR;
        case GAME_INVENTORY_RESULT_OUT_OF_SPACE:
        case GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY:
            return GAME_CONSTRUCTION_RESULT_INVENTORY_ERROR;
        default:
            return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }
}

static GameConstructionResult game_construction_map_haul_job(GameHaulJobResult result) {
    if (result == GAME_HAUL_JOB_RESULT_OK) {
        return GAME_CONSTRUCTION_RESULT_OK;
    }

    if (result == GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT || result == GAME_HAUL_JOB_RESULT_INVALID_HANDLE
        || result == GAME_HAUL_JOB_RESULT_NO_SLOT || result == GAME_HAUL_JOB_RESULT_STALE_PATH
        || result == GAME_HAUL_JOB_RESULT_PATH_FAILED || result == GAME_HAUL_JOB_RESULT_INVENTORY_ERROR
        || result == GAME_HAUL_JOB_RESULT_JOB_BOARD_ERROR || result == GAME_HAUL_JOB_RESULT_INVALID_STATE) {
        return GAME_CONSTRUCTION_RESULT_HAUL_ERROR;
    }
    return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
}

static GameConstructionResult game_construction_map_structure(GameStructureResult result) {
    if (result == GAME_STRUCTURE_RESULT_OK) {
        return GAME_CONSTRUCTION_RESULT_OK;
    }
    if (result == GAME_STRUCTURE_RESULT_OUT_OF_SPACE || result == GAME_STRUCTURE_RESULT_ALREADY_USED
        || result == GAME_STRUCTURE_RESULT_NO_SPACE) {
        return GAME_CONSTRUCTION_RESULT_STRUCTURE_ERROR;
    }
    return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
}

uint32_t game_construction_required_total(const GameConstructionWorksite *worksite) {
    if (!worksite) {
        return 0u;
    }

    uint64_t total = 0u;
    for (size_t i = 0u; i < worksite->requirement_count; ++i) {
        total += worksite->requirements[i].required_amount;
        if (total > UINT32_MAX) {
            return UINT32_MAX;
        }
    }
    return (uint32_t)total;
}

uint32_t game_construction_delivered_total(const GameConstructionWorksite *worksite) {
    if (!worksite) {
        return 0u;
    }

    uint64_t total = 0u;
    for (size_t i = 0u; i < worksite->requirement_count; ++i) {
        total += worksite->requirements[i].delivered_amount;
        if (total > UINT32_MAX) {
            return UINT32_MAX;
        }
    }
    return (uint32_t)total;
}

static GameConstructionResult game_construction_next_requirement(
    const GameConstructionWorksite *site,
    uint32_t *out_index
) {
    if (!site || !out_index) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    for (uint32_t index = 0u; index < site->requirement_count; ++index) {
        if (site->requirements[index].delivered_amount < site->requirements[index].required_amount) {
            *out_index = index;
            return GAME_CONSTRUCTION_RESULT_OK;
        }
    }

    return GAME_CONSTRUCTION_RESULT_INVALID_STATE;
}

static GameConstructionResult game_construction_request_delivery(
    GameConstructionWorksite *site,
    GameHaulJobSystem *haul_system,
    GameJobBoard *board,
    uint64_t current_tick
) {
    if (!site || !haul_system || !board) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    uint32_t requirement_index = 0u;
    if (game_construction_next_requirement(site, &requirement_index) != GAME_CONSTRUCTION_RESULT_OK) {
        return GAME_CONSTRUCTION_RESULT_INVALID_STATE;
    }

    uint32_t required = site->requirements[requirement_index].required_amount;
    uint32_t delivered = site->requirements[requirement_index].delivered_amount;
    uint32_t remaining = required - delivered;
    uint32_t batch = remaining;
    if (batch > site->worker_capacity) {
        batch = site->worker_capacity;
    }

    GameHaulJobCreateInfo haul_info = {
        .worker = site->assigned_worker,
        .route_request = site->haul_route_request,
        .source_owner = site->resource_source_owner,
        .destination_owner = site->resource_destination_owner,
        .source_tile = site->haul_start_tile,
        .destination_tile = site->haul_end_tile,
        .resource_id = site->requirements[requirement_index].resource_id,
        .required_amount = batch,
        .worker_capacity = site->worker_capacity,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
    };

    GameHaulJobHandle haul_handle = {0u, 0u};
    GameHaulJobResult haul_result = game_haul_job_create(
        haul_system,
        NULL,
        board,
        current_tick,
        &haul_info,
        &haul_handle
    );
    if (haul_result != GAME_HAUL_JOB_RESULT_OK) {
        return game_construction_map_haul_job(haul_result);
    }

    site->active_haul = haul_handle;
    site->active_requirement_index = requirement_index;
    site->state = GAME_CONSTRUCTION_STATE_DELIVERING;
    if (game_entity_id_is_valid(site->assigned_worker)) {
        (void)game_haul_job_set_worker(haul_system, haul_handle, site->assigned_worker);
    }
    return GAME_CONSTRUCTION_RESULT_OK;
}

static GameConstructionResult game_construction_settle_haul_job(
    GameConstructionWorksite *site,
    GameHaulJobSystem *haul_system,
    GameJobBoard *board,
    GameConstructionAuditLog *audit_log,
    uint64_t current_tick
) {
    if (!site || !haul_system || !board) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    if (site->active_haul.slot == 0u && site->active_haul.version == 0u) {
        return GAME_CONSTRUCTION_RESULT_INVALID_STATE;
    }

    GameHaulJob status = {0};
    GameHaulJobResult status_result = game_haul_job_status(haul_system, site->active_haul, &status);
    if (status_result != GAME_HAUL_JOB_RESULT_OK) {
        return game_construction_map_haul_job(status_result);
    }

    if (status.state == GAME_HAUL_JOB_STATE_DONE) {
        if (site->active_requirement_index >= site->requirement_count) {
            site->active_haul = (GameHaulJobHandle){0u, 0u};
            site->active_requirement_index = UINT32_MAX;
            site->state = GAME_CONSTRUCTION_STATE_STALLED;
            return GAME_CONSTRUCTION_RESULT_OK;
        }

        uint32_t index = site->active_requirement_index;
        uint32_t next = site->requirements[index].delivered_amount + status.delivered_amount;
        site->requirements[index].delivered_amount = next;
        GameHaulJobHandle finished_haul = site->active_haul;
        site->active_haul = (GameHaulJobHandle){0u, 0u};
        site->active_requirement_index = UINT32_MAX;
        site->state = GAME_CONSTRUCTION_STATE_PLANNING;
        (void)game_haul_job_clear(haul_system, board, finished_haul);
        game_construction_record_audit(
            audit_log,
            site->handle,
            current_tick,
            site->state,
            GAME_CONSTRUCTION_AUDIT_DELIVERY_COMPLETED,
            index,
            status.delivered_amount,
            site->progress_ticks
        );
        return GAME_CONSTRUCTION_RESULT_OK;
    }

    if (status.state == GAME_HAUL_JOB_STATE_STALLED || status.state == GAME_HAUL_JOB_STATE_ABORTED) {
        site->state = GAME_CONSTRUCTION_STATE_STALLED;
        game_construction_record_audit(
            audit_log,
            site->handle,
            current_tick,
            site->state,
            GAME_CONSTRUCTION_AUDIT_STALLED,
            site->active_requirement_index,
            0u,
            site->progress_ticks
        );
        return GAME_CONSTRUCTION_RESULT_HAUL_ERROR;
    }

    return GAME_CONSTRUCTION_RESULT_OK;
}

GameConstructionResult game_construction_create(
    GameConstructionSystem *system,
    GameJobBoard *board,
    GameInventory *inventory,
    uint64_t current_tick,
    const GameConstructionCreateInfo *info,
    GameConstructionWorksiteHandle *out_handle
) {
    if (!system || !board || !inventory || !out_handle || !info || !info->requirements || info->requirement_count == 0u
        || info->structure_definition_id == 0u
        || !game_inventory_owner_is_valid(info->resource_source_owner)
        || !game_inventory_owner_is_valid(info->resource_destination_owner)) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    if (info->requirement_count > GAME_CONSTRUCTION_MAX_REQUIREMENTS) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    size_t slot_index = system->capacity;
    for (size_t i = 0u; i < system->capacity; ++i) {
        if (!system->sites[i].in_use) {
            slot_index = i;
            break;
        }
    }
    if (slot_index == system->capacity) {
        return GAME_CONSTRUCTION_RESULT_NO_SLOT;
    }

    GameConstructionWorksite *site = &system->sites[slot_index];
    if (!game_entity_id_is_valid(info->assigned_worker)) {
        site->assigned_worker = (GameEntityId){0u, 0u};
    } else {
        site->assigned_worker = info->assigned_worker;
    }

    site->handle = (GameConstructionWorksiteHandle){.slot = (uint32_t)slot_index, .version = site->handle.version == 0u ? 1u : site->handle.version};
    site->stable_id = system->next_stable_id;
    site->in_use = true;
    site->state = GAME_CONSTRUCTION_STATE_PLANNING;
    site->target_tile = info->target_tile;
    site->structure_definition_id = info->structure_definition_id;
    site->haul_start_tile = info->target_tile;
    site->haul_end_tile = info->target_tile;
    site->resource_source_owner = info->resource_source_owner;
    site->resource_destination_owner = info->resource_destination_owner;
    site->required_work_ticks = info->required_work_ticks == 0u ? 1u : info->required_work_ticks;
    site->progress_ticks = 0u;
    site->active_requirement_index = UINT32_MAX;
    site->worker_capacity = info->worker_capacity == 0u ? 1u : info->worker_capacity;
    site->haul_route_request = info->haul_route_request;
    site->work_order = (GameJobOrderHandle){0u, 0u};
    site->active_haul = (GameHaulJobHandle){0u, 0u};
    site->has_structure_placement = false;
    site->structure_placement = (GameStructurePlacementHandle){0u, 0u};
    site->requirement_count = (uint32_t)info->requirement_count;

    uint32_t required_role_flags = info->required_role_flags == 0u ? GAME_WORKER_ROLE_STANDARD : info->required_role_flags;
    uint32_t required_resource_flags = info->required_resource_flags;
    uint32_t duration_min = info->duration_min_ticks == 0u ? 1u : info->duration_min_ticks;
    uint32_t duration_max = info->duration_max_ticks == 0u ? duration_min : info->duration_max_ticks;
    if (duration_max < duration_min) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    for (uint32_t i = 0u; i < site->requirement_count; ++i) {
        site->requirements[i] = info->requirements[i];
        if (site->requirements[i].delivered_amount > site->requirements[i].required_amount) {
            site->requirements[i].delivered_amount = 0u;
        }
    }

    GameJobOrderHandle board_handle = {0u, 0u};
    GameJobBoardResult board_result = game_job_board_create_order(
        board,
        current_tick,
        site->target_tile,
        required_role_flags,
        required_resource_flags,
        duration_min,
        duration_max,
        &board_handle
    );
    if (board_result != GAME_JOB_BOARD_RESULT_OK) {
        site->in_use = false;
        return game_construction_map_job_board(board_result);
    }
    site->work_order = board_handle;

    *out_handle = site->handle;
    if (system->next_stable_id == UINT32_MAX) {
        system->next_stable_id = 1u;
    } else {
        ++system->next_stable_id;
    }

    if (system->next_stable_id == 0u) {
        system->next_stable_id = 1u;
    }

    return GAME_CONSTRUCTION_RESULT_OK;
}

GameConstructionResult game_construction_status(
    const GameConstructionSystem *system,
    GameConstructionWorksiteHandle handle,
    GameConstructionWorksite *out_worksite
) {
    if (!system || !out_worksite) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    const GameConstructionWorksite *site = game_construction_lookup_slot_const(system, handle);
    if (!site) {
        return GAME_CONSTRUCTION_RESULT_INVALID_HANDLE;
    }

    *out_worksite = *site;
    return GAME_CONSTRUCTION_RESULT_OK;
}

GameConstructionResult game_construction_set_worker(
    GameConstructionSystem *system,
    GameConstructionWorksiteHandle handle,
    GameEntityId worker
) {
    if (!system) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    GameConstructionWorksite *site = game_construction_lookup_slot(system, handle);
    if (!site) {
        return GAME_CONSTRUCTION_RESULT_INVALID_HANDLE;
    }

    site->assigned_worker = worker;
    return GAME_CONSTRUCTION_RESULT_OK;
}

GameConstructionResult game_construction_update(
    GameConstructionSystem *system,
    GameJobBoard *board,
    GameInventory *inventory,
    GameHaulJobSystem *haul_system,
    GameStructureSystem *structure_system,
    GameWorldMap *map,
    uint64_t current_tick,
    GameConstructionWorksiteHandle handle,
    GameEventLog *event_log,
    GameConstructionAuditLog *audit_log
) {
    (void)inventory;
    (void)event_log;

    if (!system || !board || !haul_system || !structure_system || !map) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    GameConstructionWorksite *site = game_construction_lookup_slot(system, handle);
    if (!site) {
        return GAME_CONSTRUCTION_RESULT_INVALID_HANDLE;
    }

    if (site->state == GAME_CONSTRUCTION_STATE_DONE || site->state == GAME_CONSTRUCTION_STATE_ABORTED) {
        return GAME_CONSTRUCTION_RESULT_OK;
    }

    if (site->active_haul.slot != 0u || site->active_haul.version != 0u) {
        GameConstructionResult settle = game_construction_settle_haul_job(site, haul_system, board, audit_log, current_tick);
        if (settle != GAME_CONSTRUCTION_RESULT_OK && settle != GAME_CONSTRUCTION_RESULT_HAUL_ERROR) {
            return settle;
        }
        if (settle == GAME_CONSTRUCTION_RESULT_OK && site->active_haul.slot == 0u && site->active_haul.version == 0u) {
            return GAME_CONSTRUCTION_RESULT_OK;
        }
    }

    uint32_t next_requirement = UINT32_MAX;
    if (game_construction_next_requirement(site, &next_requirement) != GAME_CONSTRUCTION_RESULT_OK) {
        next_requirement = UINT32_MAX;
    }

    if (next_requirement != UINT32_MAX) {
        site->state = GAME_CONSTRUCTION_STATE_PLANNING;
        if (site->active_haul.slot == 0u && site->active_haul.version == 0u) {
            GameConstructionResult request = game_construction_request_delivery(site, haul_system, board, current_tick);
            if (request != GAME_CONSTRUCTION_RESULT_OK) {
                site->state = GAME_CONSTRUCTION_STATE_STALLED;
                return request;
            }
            game_construction_record_audit(
                audit_log,
                site->handle,
                current_tick,
                site->state,
                GAME_CONSTRUCTION_AUDIT_DELIVERY_STARTED,
                site->active_requirement_index,
                0u,
                site->progress_ticks
            );
        }
        return GAME_CONSTRUCTION_RESULT_OK;
    }

    if (site->required_work_ticks > 0u && site->progress_ticks < site->required_work_ticks) {
        site->state = GAME_CONSTRUCTION_STATE_BUILDING;
        site->progress_ticks += 1u;
        game_construction_record_audit(
            audit_log,
            site->handle,
            current_tick,
            site->state,
            GAME_CONSTRUCTION_AUDIT_BUILDING,
            UINT32_MAX,
            0u,
            site->progress_ticks
        );

        if (site->progress_ticks < site->required_work_ticks) {
            return GAME_CONSTRUCTION_RESULT_OK;
        }

        GameStructurePlacementHandle placement = {0u, 0u};
        GameStructureResult structure_result = game_structure_place(structure_system, map, site->structure_definition_id, site->target_tile, &placement);
        if (structure_result != GAME_STRUCTURE_RESULT_OK) {
            return game_construction_map_structure(structure_result);
        }

        site->has_structure_placement = true;
        site->structure_placement = placement;
        site->state = GAME_CONSTRUCTION_STATE_DONE;
        (void)game_job_board_transition(board, site->work_order, GAME_JOB_BOARD_STATE_DONE);
        game_construction_record_audit(
            audit_log,
            site->handle,
            current_tick,
            site->state,
            GAME_CONSTRUCTION_AUDIT_COMPLETED,
            UINT32_MAX,
            0u,
            site->progress_ticks
        );
        if (event_log) {
            (void)game_event_log_append(event_log, current_tick, GAME_EVENT_TYPE_SIMULATION, site->assigned_worker, GAME_EVENT_LOG_INVALID_PARENT_ID, NULL);
        }
        return GAME_CONSTRUCTION_RESULT_OK;
    }

    return GAME_CONSTRUCTION_RESULT_OK;
}

GameConstructionResult game_construction_abort(
    GameConstructionSystem *system,
    GameJobBoard *board,
    GameHaulJobSystem *haul_system,
    GameInventory *inventory,
    GameConstructionWorksiteHandle handle,
    uint64_t current_tick,
    GameEventLog *event_log,
    GameConstructionAuditLog *audit_log
) {
    if (!system || !board || !haul_system || !inventory) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    GameConstructionWorksite *site = game_construction_lookup_slot(system, handle);
    if (!site) {
        return GAME_CONSTRUCTION_RESULT_INVALID_HANDLE;
    }

    if (site->active_haul.slot != 0u || site->active_haul.version != 0u) {
        (void)game_haul_job_abort(haul_system, inventory, board, site->active_haul, current_tick, event_log, NULL);
        site->active_haul = (GameHaulJobHandle){0u, 0u};
        site->active_requirement_index = UINT32_MAX;
    }

    (void)game_job_board_transition(board, site->work_order, GAME_JOB_BOARD_STATE_ABORTED);
    game_construction_record_audit(
        audit_log,
        site->handle,
        current_tick,
        GAME_CONSTRUCTION_STATE_ABORTED,
        GAME_CONSTRUCTION_AUDIT_ABORTED,
        UINT32_MAX,
        0u,
        site->progress_ticks
    );

    site->state = GAME_CONSTRUCTION_STATE_ABORTED;
    return GAME_CONSTRUCTION_RESULT_OK;
}

GameConstructionResult game_construction_clear(
    GameConstructionSystem *system,
    GameJobBoard *board,
    GameHaulJobSystem *haul_system,
    GameConstructionWorksiteHandle handle
) {
    if (!system || !board || !haul_system) {
        return GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT;
    }

    GameConstructionWorksite *site = game_construction_lookup_slot(system, handle);
    if (!site) {
        return GAME_CONSTRUCTION_RESULT_INVALID_HANDLE;
    }

    if (site->active_haul.slot != 0u || site->active_haul.version != 0u) {
        (void)game_haul_job_clear(haul_system, board, site->active_haul);
        site->active_haul = (GameHaulJobHandle){0u, 0u};
    }

    if (site->has_structure_placement && site->structure_placement.version != 0u) {
        site->has_structure_placement = false;
    }

    site->in_use = false;
    site->handle.version = game_construction_next_version(site->handle.version);
    site->state = GAME_CONSTRUCTION_STATE_ABORTED;
    site->stable_id = 0u;
    site->target_tile = (GameHexAxial){0, 0};
    site->structure_definition_id = 0u;
    site->assigned_worker = (GameEntityId){0u, 0u};
    site->haul_start_tile = (GameHexAxial){0, 0};
    site->haul_end_tile = (GameHexAxial){0, 0};
    site->resource_source_owner = (GameInventoryOwner){0};
    site->resource_destination_owner = (GameInventoryOwner){0};
    site->required_work_ticks = 0u;
    site->progress_ticks = 0u;
    site->active_requirement_index = UINT32_MAX;
    site->worker_capacity = 0u;
    site->haul_route_request = (GamePathRequestHandle){0u, 0u};
    site->work_order = (GameJobOrderHandle){0u, 0u};
    site->structure_placement = (GameStructurePlacementHandle){0u, 0u};
    site->has_structure_placement = false;
    site->requirement_count = 0u;
    return GAME_CONSTRUCTION_RESULT_OK;
}

uint32_t game_construction_progress(const GameConstructionSystem *system, GameConstructionWorksiteHandle handle) {
    const GameConstructionWorksite *site = game_construction_lookup_slot_const(system, handle);
    if (!site || site->state != GAME_CONSTRUCTION_STATE_BUILDING || site->required_work_ticks == 0u) {
        return 0u;
    }

    return (site->progress_ticks * 100u) / site->required_work_ticks;
}
