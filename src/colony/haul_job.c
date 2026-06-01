#include "colony/haul_job.h"

#include <stdint.h>

static bool game_haul_job_handle_matches(const GameHaulJob *slot, GameHaulJobHandle handle) {
    return slot->in_use && slot->handle.slot == handle.slot && slot->handle.version == handle.version;
}

static GameHaulJob *game_haul_job_lookup_slot(GameHaulJobSystem *system, GameHaulJobHandle handle) {
    if (!system || !system->slots || handle.slot >= system->capacity) {
        return NULL;
    }

    GameHaulJob *slot = &system->slots[handle.slot];
    if (!game_haul_job_handle_matches(slot, handle)) {
        return NULL;
    }
    return slot;
}

static const GameHaulJob *game_haul_job_lookup_slot_const(const GameHaulJobSystem *system, GameHaulJobHandle handle) {
    if (!system || !system->slots || handle.slot >= system->capacity) {
        return NULL;
    }

    const GameHaulJob *slot = &system->slots[handle.slot];
    if (!game_haul_job_handle_matches(slot, handle)) {
        return NULL;
    }
    return slot;
}

static bool game_haul_job_record_audit(
    GameHaulJobAuditLog *audit_log,
    GameHaulJobHandle handle,
    uint64_t tick,
    GameEntityId worker,
    GameHaulJobState state,
    GameHaulJobAuditReason reason,
    uint32_t requested_amount,
    uint32_t delivered_amount
) {
    if (!audit_log || !audit_log->entries || audit_log->count >= audit_log->capacity) {
        return false;
    }

    audit_log->entries[audit_log->count++] = (GameHaulJobAuditEntry){
        .job = handle,
        .tick = tick,
        .worker = worker,
        .state = state,
        .reason = reason,
        .requested_amount = requested_amount,
        .delivered_amount = delivered_amount,
    };
    return true;
}

void game_haul_job_audit_init(GameHaulJobAuditLog *log, GameHaulJobAuditEntry *entries, size_t capacity) {
    if (!log) {
        return;
    }

    log->entries = entries;
    log->capacity = entries ? capacity : 0u;
    log->count = 0u;
}

GameHaulJobResult game_haul_job_init(GameHaulJobSystem *system, GameHaulJob *slots, size_t slot_count) {
    if (!system || !slots || slot_count == 0u) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    system->capacity = slot_count;
    system->slots = slots;
    system->next_stable_id = 1u;

    for (size_t i = 0u; i < slot_count; ++i) {
        slots[i] = (GameHaulJob){
            .in_use = false,
            .handle = {(uint32_t)i, 1u},
            .stable_id = 0u,
            .source_owner = {0u, {0u, 0u}, 0u},
            .destination_owner = {0u, {0u, 0u}, 0u},
            .resource_id = 0u,
            .requested_amount = 0u,
            .delivered_amount = 0u,
            .worker_capacity = 0u,
            .source_tile = {0, 0},
            .destination_tile = {0, 0},
            .state = GAME_HAUL_JOB_STATE_ABORTED,
            .route_request = {0u, 0u},
            .source_reservation = {0u, 0u},
            .work_order = {0u, 0u},
            .route_result_version = 0u,
            .worker = {0u, 0u},
        };
    }

    return GAME_HAUL_JOB_RESULT_OK;
}

size_t game_haul_job_active_count(const GameHaulJobSystem *system) {
    if (!system || !system->slots) {
        return 0u;
    }

    size_t count = 0u;
    for (size_t i = 0u; i < system->capacity; ++i) {
        if (system->slots[i].in_use && system->slots[i].state != GAME_HAUL_JOB_STATE_DONE
            && system->slots[i].state != GAME_HAUL_JOB_STATE_ABORTED) {
            ++count;
        }
    }
    return count;
}

static GameHaulJobResult game_haul_job_map_inventory(GameInventoryResult result) {
    switch (result) {
        case GAME_INVENTORY_RESULT_OK:
            return GAME_HAUL_JOB_RESULT_OK;
        case GAME_INVENTORY_RESULT_INVALID_ARGUMENT:
        case GAME_INVENTORY_RESULT_INVALID_OWNER:
        case GAME_INVENTORY_RESULT_INVALID_HANDLE:
        case GAME_INVENTORY_RESULT_ENTRY_NOT_FOUND:
        case GAME_INVENTORY_RESULT_RESERVATION_STALE:
        case GAME_INVENTORY_RESULT_OUT_OF_SPACE:
        case GAME_INVENTORY_RESULT_RESERVATION_FULL:
        case GAME_INVENTORY_RESULT_RESERVATION_NOT_FOUND:
            return GAME_HAUL_JOB_RESULT_INVENTORY_ERROR;
        case GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY:
            return GAME_HAUL_JOB_RESULT_INVENTORY_ERROR;
        default:
            return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }
}

static GameHaulJobResult game_haul_job_map_jobboard(GameJobBoardResult result) {
    switch (result) {
        case GAME_JOB_BOARD_RESULT_OK:
            return GAME_HAUL_JOB_RESULT_OK;
        case GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT:
        case GAME_JOB_BOARD_RESULT_INVALID_HANDLE:
        case GAME_JOB_BOARD_RESULT_NO_SLOT:
        case GAME_JOB_BOARD_RESULT_ALREADY_RESERVED:
        case GAME_JOB_BOARD_RESULT_INVALID_STATE:
            return GAME_HAUL_JOB_RESULT_JOB_BOARD_ERROR;
        default:
            return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }
}

GameHaulJobResult game_haul_job_create(
    GameHaulJobSystem *system,
    GameInventory *inventory,
    GameJobBoard *board,
    uint64_t current_tick,
    const GameHaulJobCreateInfo *info,
    GameHaulJobHandle *out_handle
) {
    (void)inventory;
    if (!system || !board || !info || !out_handle) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    if (!game_inventory_owner_is_valid(info->source_owner) || !game_inventory_owner_is_valid(info->destination_owner)) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    if (info->resource_id == 0u || info->required_amount == 0u || info->worker_capacity == 0u) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    if (system->next_stable_id == 0u) {
        return GAME_HAUL_JOB_RESULT_INVALID_STATE;
    }

    size_t slot_index = system->capacity;
    for (size_t i = 0u; i < system->capacity; ++i) {
        if (!system->slots[i].in_use) {
            slot_index = i;
            break;
        }
    }
    if (slot_index == system->capacity) {
        return GAME_HAUL_JOB_RESULT_NO_SLOT;
    }

    GameJobOrderHandle board_handle = {0u, 0u};
    GameJobBoardResult board_result = game_job_board_create_order(
        board,
        current_tick,
        info->source_tile,
        info->required_role_flags,
        info->resource_id,
        info->duration_min_ticks,
        info->duration_max_ticks,
        &board_handle
    );
    if (board_result != GAME_JOB_BOARD_RESULT_OK) {
        return game_haul_job_map_jobboard(board_result);
    }

    GameHaulJob *slot = &system->slots[slot_index];
    slot->in_use = true;
    slot->handle.slot = (uint32_t)slot_index;
    slot->handle.version = slot->handle.version == 0u ? 1u : slot->handle.version;
    slot->stable_id = system->next_stable_id;
    ++system->next_stable_id;
    if (system->next_stable_id == 0u) {
        system->next_stable_id = 1u;
    }

    slot->worker = game_entity_invalid_id();
    slot->source_owner = info->source_owner;
    slot->destination_owner = info->destination_owner;
    slot->resource_id = info->resource_id;
    slot->requested_amount = info->required_amount <= info->worker_capacity ? info->required_amount : info->worker_capacity;
    slot->delivered_amount = 0u;
    slot->worker_capacity = info->worker_capacity;
    slot->source_tile = info->source_tile;
    slot->destination_tile = info->destination_tile;
    slot->state = GAME_HAUL_JOB_STATE_RESERVE_SOURCE;
    slot->route_request = info->route_request;
    slot->route_result_version = 0u;
    slot->source_reservation = (GameInventoryReservationHandle){0u, 0u};
    slot->work_order = board_handle;
    *out_handle = slot->handle;
    return GAME_HAUL_JOB_RESULT_OK;
}

GameHaulJobResult game_haul_job_status(
    const GameHaulJobSystem *system,
    GameHaulJobHandle handle,
    GameHaulJob *out_job
) {
    if (!system || !out_job) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    const GameHaulJob *slot = game_haul_job_lookup_slot_const(system, handle);
    if (!slot) {
        return GAME_HAUL_JOB_RESULT_INVALID_HANDLE;
    }

    *out_job = *slot;
    return GAME_HAUL_JOB_RESULT_OK;
}

GameHaulJobResult game_haul_job_set_worker(GameHaulJobSystem *system, GameHaulJobHandle handle, GameEntityId worker) {
    if (!system || !game_entity_id_is_valid(worker)) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    GameHaulJob *slot = game_haul_job_lookup_slot(system, handle);
    if (!slot) {
        return GAME_HAUL_JOB_RESULT_INVALID_HANDLE;
    }

    slot->worker = worker;
    return GAME_HAUL_JOB_RESULT_OK;
}

static bool game_haul_job_validate_transition(GameHaulJobState current, GameHaulJobState next) {
    if (current == next) {
        return true;
    }

    switch (current) {
        case GAME_HAUL_JOB_STATE_RESERVE_SOURCE:
            return next == GAME_HAUL_JOB_STATE_PICKUP || next == GAME_HAUL_JOB_STATE_STALLED || next == GAME_HAUL_JOB_STATE_ABORTED;
        case GAME_HAUL_JOB_STATE_PICKUP:
            return next == GAME_HAUL_JOB_STATE_IN_TRANSIT || next == GAME_HAUL_JOB_STATE_STALLED
                   || next == GAME_HAUL_JOB_STATE_ABORTED;
        case GAME_HAUL_JOB_STATE_IN_TRANSIT:
            return next == GAME_HAUL_JOB_STATE_DELIVERY || next == GAME_HAUL_JOB_STATE_STALLED
                   || next == GAME_HAUL_JOB_STATE_ABORTED;
        case GAME_HAUL_JOB_STATE_DELIVERY:
            return next == GAME_HAUL_JOB_STATE_DONE || next == GAME_HAUL_JOB_STATE_STALLED || next == GAME_HAUL_JOB_STATE_ABORTED;
        case GAME_HAUL_JOB_STATE_STALLED:
            return next == GAME_HAUL_JOB_STATE_RESERVE_SOURCE || next == GAME_HAUL_JOB_STATE_ABORTED;
        case GAME_HAUL_JOB_STATE_DONE:
        case GAME_HAUL_JOB_STATE_ABORTED:
            return false;
        default:
            return false;
    }
}

GameHaulJobResult game_haul_job_assess_route(
    const GameHaulJobSystem *system,
    const GamePathService *path_service,
    GameHaulJobHandle handle,
    GameHaulJobPathStatus *out_status
) {
    if (!system || !out_status) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }
    if (!path_service) {
        *out_status = GAME_HAUL_JOB_PATH_STATUS_MISSING;
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    const GameHaulJob *job = game_haul_job_lookup_slot_const(system, handle);
    if (!job) {
        return GAME_HAUL_JOB_RESULT_INVALID_HANDLE;
    }

    if (job->route_request.slot == 0u && job->route_request.version == 0u) {
        *out_status = GAME_HAUL_JOB_PATH_STATUS_MISSING;
        return GAME_HAUL_JOB_RESULT_PATH_FAILED;
    }

    GamePathServiceStatus status = {0};
    GamePathServiceResult psr = game_path_service_status(path_service, job->route_request, &status);
    if (psr != GAME_PATH_SERVICE_RESULT_OK) {
        *out_status = GAME_HAUL_JOB_PATH_STATUS_MISSING;
        return GAME_HAUL_JOB_RESULT_INVALID_HANDLE;
    }

    if (!status.has_result_path || status.result_path_length == 0u) {
        *out_status = GAME_HAUL_JOB_PATH_STATUS_FAILED;
        return GAME_HAUL_JOB_RESULT_PATH_FAILED;
    }

    if (status.state == GAME_PATH_SERVICE_STATE_RESOLVED) {
        if (job->route_result_version != 0u && job->route_result_version != status.result_version) {
            *out_status = GAME_HAUL_JOB_PATH_STATUS_STALE;
            return GAME_HAUL_JOB_RESULT_STALE_PATH;
        }
        *out_status = GAME_HAUL_JOB_PATH_STATUS_OK;
        return GAME_HAUL_JOB_RESULT_OK;
    }

    if (status.state == GAME_PATH_SERVICE_STATE_FAILED || status.state == GAME_PATH_SERVICE_STATE_CANCELLED || status.state == GAME_PATH_SERVICE_STATE_EXPIRED) {
        *out_status = GAME_HAUL_JOB_PATH_STATUS_FAILED;
        return GAME_HAUL_JOB_RESULT_PATH_FAILED;
    }

    *out_status = GAME_HAUL_JOB_PATH_STATUS_UNKNOWN;
    return GAME_HAUL_JOB_RESULT_INVALID_STATE;
}

static GameHaulJobResult game_haul_job_release_reservation(GameHaulJob *job, GameInventory *inventory) {
    if (!job || !inventory) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    if (job->source_reservation.slot == 0u && job->source_reservation.version == 0u) {
        return GAME_HAUL_JOB_RESULT_OK;
    }

    GameInventoryResult inventory_result = game_inventory_release_reservation(inventory, job->source_reservation);
    if (inventory_result == GAME_INVENTORY_RESULT_OK || inventory_result == GAME_INVENTORY_RESULT_INVALID_HANDLE
        || inventory_result == GAME_INVENTORY_RESULT_RESERVATION_STALE) {
        job->source_reservation = (GameInventoryReservationHandle){0u, 0u};
        if (inventory_result == GAME_INVENTORY_RESULT_OK || inventory_result == GAME_INVENTORY_RESULT_RESERVATION_STALE) {
            return GAME_HAUL_JOB_RESULT_OK;
        }
        return game_haul_job_map_inventory(inventory_result);
    }

    return game_haul_job_map_inventory(inventory_result);
}

static GameHaulJobResult game_haul_job_ensure_source_reservation(GameHaulJob *job, GameInventory *inventory) {
    if (!job || !inventory) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    if (job->source_reservation.slot != 0u || job->source_reservation.version != 0u) {
        return GAME_HAUL_JOB_RESULT_OK;
    }

    GameInventoryReservationHandle reservation = {0u, 0u};
    GameInventoryResult inventory_result = game_inventory_reserve(
        inventory,
        job->source_owner,
        job->resource_id,
        job->requested_amount,
        &reservation
    );
    if (inventory_result != GAME_INVENTORY_RESULT_OK) {
        return game_haul_job_map_inventory(inventory_result);
    }

    job->source_reservation = reservation;
    return GAME_HAUL_JOB_RESULT_OK;
}

static GameHaulJobResult game_haul_job_transfer_delivery(GameHaulJob *job, GameInventory *inventory, uint32_t *out_delivered) {
    if (!job || !inventory || !out_delivered) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    if (job->source_reservation.slot == 0u && job->source_reservation.version == 0u) {
        return GAME_HAUL_JOB_RESULT_INVENTORY_ERROR;
    }

    uint32_t to_move = job->requested_amount;
    GameInventoryResult add_result = game_inventory_add(inventory, job->destination_owner, job->resource_id, to_move, NULL);
    if (add_result != GAME_INVENTORY_RESULT_OK) {
        *out_delivered = 0u;
        return game_haul_job_map_inventory(add_result);
    }

    GameInventoryResult commit_result = game_inventory_commit_reservation(inventory, job->source_reservation, NULL);
    if (commit_result != GAME_INVENTORY_RESULT_OK) {
        uint32_t remove_amount = 0u;
        (void)game_inventory_remove(inventory, job->destination_owner, job->resource_id, to_move, &remove_amount);
        job->source_reservation = (GameInventoryReservationHandle){0u, 0u};
        *out_delivered = 0u;
        return game_haul_job_map_inventory(commit_result);
    }

    job->source_reservation = (GameInventoryReservationHandle){0u, 0u};
    *out_delivered = to_move;
    return GAME_HAUL_JOB_RESULT_OK;
}

GameHaulJobResult game_haul_job_advance(
    GameHaulJobSystem *system,
    GameJobBoard *board,
    GameInventory *inventory,
    const GamePathService *path_service,
    GameHaulJobHandle handle,
    GameHaulJobState next_state,
    uint64_t current_tick,
    GameEventLog *event_log,
    GameHaulJobAuditLog *audit_log
) {
    if (!system || !board || !inventory) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    GameHaulJob *job = game_haul_job_lookup_slot(system, handle);
    if (!job) {
        return GAME_HAUL_JOB_RESULT_INVALID_HANDLE;
    }

    if (!game_haul_job_validate_transition(job->state, next_state)) {
        return GAME_HAUL_JOB_RESULT_INVALID_STATE;
    }

    GameHaulJobResult result = GAME_HAUL_JOB_RESULT_OK;
    GameHaulJobAuditReason reason = GAME_HAUL_JOB_AUDIT_CREATED;
    GameHaulJobState applied_state = next_state;

    if (next_state == GAME_HAUL_JOB_STATE_RESERVE_SOURCE) {
        result = game_haul_job_ensure_source_reservation(job, inventory);
        reason = result == GAME_HAUL_JOB_RESULT_OK ? GAME_HAUL_JOB_AUDIT_SOURCE_RESERVED : GAME_HAUL_JOB_AUDIT_STALLED;
        if (result != GAME_HAUL_JOB_RESULT_OK) {
            applied_state = GAME_HAUL_JOB_STATE_STALLED;
        }
    } else if (next_state == GAME_HAUL_JOB_STATE_PICKUP) {
        reason = GAME_HAUL_JOB_AUDIT_PICKUP;
        GameHaulJobResult ensure = game_haul_job_ensure_source_reservation(job, inventory);
        if (ensure != GAME_HAUL_JOB_RESULT_OK) {
            reason = GAME_HAUL_JOB_AUDIT_STALLED;
            applied_state = GAME_HAUL_JOB_STATE_STALLED;
            result = ensure;
        }
    } else if (next_state == GAME_HAUL_JOB_STATE_IN_TRANSIT) {
        if (!path_service) {
            reason = GAME_HAUL_JOB_AUDIT_STALLED;
            result = GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
            applied_state = GAME_HAUL_JOB_STATE_STALLED;
        } else {
            GameHaulJobPathStatus path_status = GAME_HAUL_JOB_PATH_STATUS_UNKNOWN;
            result = game_haul_job_assess_route(system, path_service, handle, &path_status);
            if (result != GAME_HAUL_JOB_RESULT_OK) {
                reason = GAME_HAUL_JOB_AUDIT_STALLED;
                applied_state = GAME_HAUL_JOB_STATE_STALLED;
            } else {
                reason = GAME_HAUL_JOB_AUDIT_IN_TRANSIT;
                GamePathServiceStatus status = {0};
                if (game_path_service_status(path_service, job->route_request, &status) == GAME_PATH_SERVICE_RESULT_OK) {
                    job->route_result_version = status.result_version;
                }
            }
        }
    } else if (next_state == GAME_HAUL_JOB_STATE_DELIVERY) {
        reason = GAME_HAUL_JOB_AUDIT_DELIVERED;
        uint32_t delivered_now = 0u;
        result = game_haul_job_transfer_delivery(job, inventory, &delivered_now);
        if (result != GAME_HAUL_JOB_RESULT_OK) {
            reason = GAME_HAUL_JOB_AUDIT_STALLED;
            applied_state = GAME_HAUL_JOB_STATE_STALLED;
        } else {
            job->delivered_amount += delivered_now;
            if (job->delivered_amount > job->requested_amount) {
                job->delivered_amount = job->requested_amount;
            }
        }
    } else if (next_state == GAME_HAUL_JOB_STATE_STALLED) {
        reason = GAME_HAUL_JOB_AUDIT_STALLED;
        (void)game_haul_job_release_reservation(job, inventory);
        result = GAME_HAUL_JOB_RESULT_OK;
    } else if (next_state == GAME_HAUL_JOB_STATE_DONE) {
        reason = GAME_HAUL_JOB_AUDIT_DELIVERED;
        GameJobBoardResult transition_result = game_job_board_transition(board, job->work_order, GAME_JOB_BOARD_STATE_DONE);
        if (transition_result != GAME_JOB_BOARD_RESULT_OK && transition_result != GAME_JOB_BOARD_RESULT_INVALID_STATE) {
            return game_haul_job_map_jobboard(transition_result);
        }
        (void)game_haul_job_release_reservation(job, inventory);
    } else if (next_state == GAME_HAUL_JOB_STATE_ABORTED) {
        reason = GAME_HAUL_JOB_AUDIT_ABORTED;
        GameJobBoardResult transition_result = game_job_board_transition(board, job->work_order, GAME_JOB_BOARD_STATE_ABORTED);
        if (transition_result != GAME_JOB_BOARD_RESULT_OK && transition_result != GAME_JOB_BOARD_RESULT_INVALID_STATE) {
            return game_haul_job_map_jobboard(transition_result);
        }
        (void)game_haul_job_release_reservation(job, inventory);
    }

    job->state = applied_state;
    game_haul_job_record_audit(
        audit_log,
        job->handle,
        current_tick,
        job->worker,
        applied_state,
        reason,
        job->requested_amount,
        job->delivered_amount
    );

    if (event_log) {
        (void)game_event_log_append(event_log, current_tick, GAME_EVENT_TYPE_SIMULATION, job->worker, GAME_EVENT_LOG_INVALID_PARENT_ID, NULL);
    }

    return result;
}

GameHaulJobResult game_haul_job_abort(
    GameHaulJobSystem *system,
    GameInventory *inventory,
    GameJobBoard *board,
    GameHaulJobHandle handle,
    uint64_t current_tick,
    GameEventLog *event_log,
    GameHaulJobAuditLog *audit_log
) {
    return game_haul_job_advance(
        system,
        board,
        inventory,
        NULL,
        handle,
        GAME_HAUL_JOB_STATE_ABORTED,
        current_tick,
        event_log,
        audit_log
    );
}

GameHaulJobResult game_haul_job_clear(GameHaulJobSystem *system, GameJobBoard *board, GameHaulJobHandle handle) {
    if (!system || !board) {
        return GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT;
    }

    GameHaulJob *job = game_haul_job_lookup_slot(system, handle);
    if (!job) {
        return GAME_HAUL_JOB_RESULT_INVALID_HANDLE;
    }

    job->in_use = false;
    if (job->work_order.slot != 0u || job->work_order.version != 0u) {
        (void)game_job_board_transition(board, job->work_order, GAME_JOB_BOARD_STATE_ABORTED);
    }

    job->handle.version = job->handle.version == 0u ? 1u : job->handle.version + 1u;
    if (job->handle.version == 0u) {
        job->handle.version = 1u;
    }
    job->stable_id = 0u;
    job->worker = game_entity_invalid_id();
    job->source_owner = (GameInventoryOwner){0u, {0u, 0u}, 0u};
    job->destination_owner = (GameInventoryOwner){0u, {0u, 0u}, 0u};
    job->resource_id = 0u;
    job->requested_amount = 0u;
    job->delivered_amount = 0u;
    job->worker_capacity = 0u;
    job->source_tile = (GameHexAxial){0, 0};
    job->destination_tile = (GameHexAxial){0, 0};
    job->state = GAME_HAUL_JOB_STATE_ABORTED;
    job->route_request = (GamePathRequestHandle){0u, 0u};
    job->route_result_version = 0u;
    job->source_reservation = (GameInventoryReservationHandle){0u, 0u};
    job->work_order = (GameJobOrderHandle){0u, 0u};
    return GAME_HAUL_JOB_RESULT_OK;
}

uint32_t game_haul_job_progress(const GameHaulJobSystem *system, GameHaulJobHandle handle) {
    const GameHaulJob *job = game_haul_job_lookup_slot_const(system, handle);
    if (!job || job->requested_amount == 0u) {
        return 0u;
    }
    return (job->delivered_amount * 100u) / job->requested_amount;
}
