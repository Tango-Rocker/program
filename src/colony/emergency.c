#include "colony/emergency.h"

#include <stdint.h>

static GameEmergencyInterruption *game_emergency_find_slot(
    GameEmergencySystem *system,
    GameEntityId worker
) {
    if (!system || !system->interruptions || system->capacity == 0u) {
        return NULL;
    }

    for (size_t i = 0u; i < system->capacity; ++i) {
        GameEmergencyInterruption *slot = &system->interruptions[i];
        if (slot->in_use && slot->worker.index == worker.index && slot->worker.generation == worker.generation) {
            return slot;
        }
    }

    return NULL;
}

static GameEmergencyInterruption *game_emergency_reserve_slot(GameEmergencySystem *system, GameEntityId worker) {
    if (!system || !system->interruptions || system->capacity == 0u) {
        return NULL;
    }

    GameEmergencyInterruption *recycled = NULL;
    for (size_t i = 0u; i < system->capacity; ++i) {
        GameEmergencyInterruption *slot = &system->interruptions[i];
        if (!slot->in_use) {
            slot->in_use = true;
            slot->worker = worker;
            slot->active = false;
            slot->resume_on_clear = false;
            slot->routine_order = (GameJobOrderHandle){0u, 0u};
            slot->emergency_order = (GameJobOrderHandle){0u, 0u};
            slot->interruption_tick = 0u;
            slot->last_threat_level = 0u;
            return slot;
        }
        if (!recycled && !slot->active) {
            recycled = slot;
        }
    }

    if (recycled) {
        recycled->worker = worker;
        recycled->active = false;
        recycled->resume_on_clear = false;
        recycled->routine_order = (GameJobOrderHandle){0u, 0u};
        recycled->emergency_order = (GameJobOrderHandle){0u, 0u};
        recycled->interruption_tick = 0u;
        recycled->last_threat_level = 0u;
        return recycled;
    }

    return NULL;
}

void game_emergency_audit_init(GameEmergencyAuditLog *log, GameEmergencyAuditEntry *entries, size_t capacity) {
    if (!log) {
        return;
    }

    log->entries = entries;
    log->capacity = entries ? capacity : 0u;
    log->count = 0u;
}

static bool game_emergency_record_audit(
    GameEmergencyAuditLog *audit_log,
    GameEntityId worker,
    GameJobOrderHandle routine_order,
    GameJobOrderHandle emergency_order,
    uint64_t tick,
    uint32_t threat_level,
    bool resume_requested,
    GameEmergencyAuditReason reason
) {
    if (!audit_log || !audit_log->entries || audit_log->count >= audit_log->capacity) {
        return false;
    }

    audit_log->entries[audit_log->count++] = (GameEmergencyAuditEntry){
        .tick = tick,
        .worker = worker,
        .routine_order = routine_order,
        .emergency_order = emergency_order,
        .reason = reason,
        .threat_level = threat_level,
        .resume_requested = resume_requested,
    };
    return true;
}

GameEmergencyResult game_emergency_init(
    GameEmergencySystem *system,
    GameEmergencyInterruption *interruptions,
    size_t capacity
) {
    if (!system || !interruptions || capacity == 0u) {
        return GAME_EMERGENCY_RESULT_INVALID_ARGUMENT;
    }

    system->capacity = capacity;
    system->interruptions = interruptions;
    for (size_t i = 0u; i < capacity; ++i) {
        interruptions[i] = (GameEmergencyInterruption){0};
    }
    return GAME_EMERGENCY_RESULT_OK;
}

static GameEmergencyResult game_emergency_map_board(GameJobBoardResult result) {
    switch (result) {
        case GAME_JOB_BOARD_RESULT_OK:
            return GAME_EMERGENCY_RESULT_OK;
        case GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT:
        case GAME_JOB_BOARD_RESULT_INVALID_HANDLE:
        case GAME_JOB_BOARD_RESULT_NO_SLOT:
        case GAME_JOB_BOARD_RESULT_ALREADY_RESERVED:
        case GAME_JOB_BOARD_RESULT_INVALID_STATE:
            return GAME_EMERGENCY_RESULT_JOB_BOARD_ERROR;
        default:
            return GAME_EMERGENCY_RESULT_INVALID_ARGUMENT;
    }
}

static GameEmergencyResult game_emergency_map_inventory(GameInventoryResult result) {
    switch (result) {
        case GAME_INVENTORY_RESULT_OK:
            return GAME_EMERGENCY_RESULT_OK;
        case GAME_INVENTORY_RESULT_INVALID_ARGUMENT:
        case GAME_INVENTORY_RESULT_INVALID_HANDLE:
        case GAME_INVENTORY_RESULT_INVALID_OWNER:
        case GAME_INVENTORY_RESULT_RESERVATION_NOT_FOUND:
        case GAME_INVENTORY_RESULT_RESERVATION_STALE:
        case GAME_INVENTORY_RESULT_RESERVATION_FULL:
        case GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY:
        case GAME_INVENTORY_RESULT_ENTRY_NOT_FOUND:
            return GAME_EMERGENCY_RESULT_INVENTORY_ERROR;
        default:
            return GAME_EMERGENCY_RESULT_INVALID_ARGUMENT;
    }
}

static GameEmergencyResult game_emergency_map_haul(GameHaulJobResult result) {
    if (result == GAME_HAUL_JOB_RESULT_OK) {
        return GAME_EMERGENCY_RESULT_OK;
    }
    if (result == GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT || result == GAME_HAUL_JOB_RESULT_INVALID_HANDLE
        || result == GAME_HAUL_JOB_RESULT_NO_SLOT || result == GAME_HAUL_JOB_RESULT_INVALID_STATE
        || result == GAME_HAUL_JOB_RESULT_INVENTORY_ERROR || result == GAME_HAUL_JOB_RESULT_JOB_BOARD_ERROR
        || result == GAME_HAUL_JOB_RESULT_PATH_FAILED || result == GAME_HAUL_JOB_RESULT_STALE_PATH) {
        return GAME_EMERGENCY_RESULT_HAUL_ERROR;
    }

    return GAME_EMERGENCY_RESULT_INVALID_ARGUMENT;
}

static void game_emergency_release_active_emergency(GameJobBoard *board, GameEmergencyInterruption *slot, bool force_abort) {
    if (!board || !slot || !slot->active) {
        return;
    }

    if (slot->emergency_order.slot != 0u || slot->emergency_order.version != 0u) {
        if (force_abort) {
            GameJobBoardResult transition = game_job_board_transition(board, slot->emergency_order, GAME_JOB_BOARD_STATE_ABORTED);
            if (transition != GAME_JOB_BOARD_RESULT_OK && transition != GAME_JOB_BOARD_RESULT_INVALID_STATE) {
                (void)transition;
            }
        }
        slot->emergency_order = (GameJobOrderHandle){0u, 0u};
    }
}

static bool game_emergency_is_empty_handle(GameJobOrderHandle handle) {
    return handle.slot == 0u && handle.version == 0u;
}

static bool game_emergency_is_empty_entity(GameEntityId entity) {
    return !game_entity_id_is_valid(entity);
}

static bool game_emergency_has_stale_order(GameJobBoard *board, GameJobOrderHandle handle, GameJobOrder *out_order) {
    if (!board || game_emergency_is_empty_handle(handle) || !out_order) {
        return true;
    }

    return game_job_board_status(board, handle, out_order) != GAME_JOB_BOARD_RESULT_OK;
}

GameEmergencyResult game_emergency_update(
    GameEmergencySystem *system,
    GameJobBoard *board,
    GameInventory *inventory,
    GameHaulJobSystem *haul_system,
    GameWorkerState *worker,
    GameHaulJobHandle active_haul_job,
    uint64_t current_tick,
    uint32_t threat_level,
    uint32_t threat_threshold,
    bool resume_when_clear,
    GameHexAxial emergency_tile,
    uint32_t emergency_role_flags,
    uint32_t emergency_required_resource_flags,
    uint32_t emergency_duration_min_ticks,
    uint32_t emergency_duration_max_ticks,
    GameEmergencyAuditLog *audit_log
) {
    (void)emergency_tile;
    if (!system || !board || !worker || game_emergency_is_empty_entity(worker->id)) {
        return GAME_EMERGENCY_RESULT_INVALID_ARGUMENT;
    }
    if (threat_threshold == 0u) {
        threat_threshold = 1u;
    }
    if (emergency_duration_min_ticks == 0u) {
        emergency_duration_min_ticks = 1u;
    }
    if (emergency_duration_max_ticks == 0u) {
        emergency_duration_max_ticks = emergency_duration_min_ticks;
    }
    if (emergency_duration_max_ticks < emergency_duration_min_ticks) {
        return GAME_EMERGENCY_RESULT_INVALID_ARGUMENT;
    }

    if (emergency_role_flags == 0u) {
        emergency_role_flags = GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY;
    }

    bool threat_active = threat_level >= threat_threshold;

    GameEmergencyInterruption *slot = game_emergency_find_slot(system, worker->id);
    if (!slot && threat_active) {
        slot = game_emergency_reserve_slot(system, worker->id);
        if (!slot) {
            return GAME_EMERGENCY_RESULT_NO_SLOT;
        }
    }

    if (!slot) {
        game_emergency_record_audit(
            audit_log,
            worker->id,
            (GameJobOrderHandle){0u, 0u},
            (GameJobOrderHandle){0u, 0u},
            current_tick,
            threat_level,
            resume_when_clear,
            GAME_EMERGENCY_AUDIT_THREAT_CLEARED_NO_INTERRUPT
        );
        return GAME_EMERGENCY_RESULT_OK;
    }

    if (!threat_active) {
        if (!slot->active) {
            slot->in_use = false;
            game_emergency_record_audit(
                audit_log,
                worker->id,
                (GameJobOrderHandle){0u, 0u},
                (GameJobOrderHandle){0u, 0u},
                current_tick,
                threat_level,
                resume_when_clear,
                GAME_EMERGENCY_AUDIT_THREAT_CLEARED_NO_INTERRUPT
            );
            return GAME_EMERGENCY_RESULT_OK;
        }

        bool resume_requested = slot->resume_on_clear;
        slot->active = false;

        GameJobOrder routine_status = {0};
        if (!game_emergency_has_stale_order(board, slot->routine_order, &routine_status)) {
            bool resume_routine = false;
            if (resume_requested && resume_when_clear) {
                if (routine_status.state == GAME_JOB_BOARD_STATE_STALLED) {
                    GameJobBoardResult reopen = game_job_board_transition(
                        board,
                        slot->routine_order,
                        GAME_JOB_BOARD_STATE_OPEN
                    );
                    if (reopen == GAME_JOB_BOARD_RESULT_OK) {
                        GameJobBoardResult reserve = game_job_board_reserve(
                            board,
                            slot->routine_order,
                            current_tick,
                            worker->id
                        );
                        if (reserve == GAME_JOB_BOARD_RESULT_OK) {
                            GameJobBoardResult resume = game_job_board_transition(
                                board,
                                slot->routine_order,
                                GAME_JOB_BOARD_STATE_IN_PROGRESS
                            );
                            if (resume == GAME_JOB_BOARD_RESULT_OK) {
                                resume_routine = true;
                                worker->current_job = slot->routine_order;
                                worker->has_current_job = true;
                            }
                        }
                    }

                    if (resume_routine) {
                        game_emergency_record_audit(
                            audit_log,
                            worker->id,
                            slot->routine_order,
                            slot->emergency_order,
                            current_tick,
                            threat_level,
                            resume_when_clear,
                            GAME_EMERGENCY_AUDIT_ROUTINE_RESUMED
                        );
                    } else {
                        GameJobBoardResult abort_result = game_job_board_transition(
                            board,
                            slot->routine_order,
                            GAME_JOB_BOARD_STATE_ABORTED
                        );
                        (void)abort_result;
                        worker->current_job = (GameJobOrderHandle){0u, 0u};
                        worker->has_current_job = false;
                        game_emergency_record_audit(
                            audit_log,
                            worker->id,
                            slot->routine_order,
                            slot->emergency_order,
                            current_tick,
                            threat_level,
                            resume_when_clear,
                            GAME_EMERGENCY_AUDIT_ROUTINE_ABORTED
                        );
                    }
                } else {
                    GameJobBoardResult abort_result = game_job_board_transition(
                        board,
                        slot->routine_order,
                        GAME_JOB_BOARD_STATE_ABORTED
                    );
                    (void)abort_result;
                    worker->current_job = (GameJobOrderHandle){0u, 0u};
                    worker->has_current_job = false;
                    game_emergency_record_audit(
                        audit_log,
                        worker->id,
                        slot->routine_order,
                        slot->emergency_order,
                        current_tick,
                        threat_level,
                        resume_when_clear,
                        GAME_EMERGENCY_AUDIT_ROUTINE_ABORTED
                    );
                }
            } else {
                GameJobBoardResult abort_result = game_job_board_transition(
                    board,
                    slot->routine_order,
                    GAME_JOB_BOARD_STATE_ABORTED
                );
                (void)abort_result;
                worker->current_job = (GameJobOrderHandle){0u, 0u};
                worker->has_current_job = false;
                game_emergency_record_audit(
                    audit_log,
                    worker->id,
                    slot->routine_order,
                    slot->emergency_order,
                    current_tick,
                    threat_level,
                    resume_when_clear,
                    GAME_EMERGENCY_AUDIT_ROUTINE_ABORTED
                );
            }
        }

        game_emergency_release_active_emergency(board, slot, true);
        slot->routine_order = (GameJobOrderHandle){0u, 0u};
        slot->emergency_order = (GameJobOrderHandle){0u, 0u};
        slot->in_use = false;
        slot->interruption_tick = 0u;
        slot->last_threat_level = 0u;
        return GAME_EMERGENCY_RESULT_OK;
    }

    if (slot->active) {
        game_emergency_record_audit(
            audit_log,
            worker->id,
            slot->routine_order,
            slot->emergency_order,
            current_tick,
            threat_level,
            resume_when_clear,
            GAME_EMERGENCY_AUDIT_ROUTINE_STALLED
        );
        worker->current_job = slot->emergency_order;
        worker->has_current_job = true;
        return GAME_EMERGENCY_RESULT_ALREADY_ACTIVE;
    }

    if (!worker->has_current_job || game_emergency_is_empty_handle(worker->current_job)) {
        game_emergency_record_audit(
            audit_log,
            worker->id,
            (GameJobOrderHandle){0u, 0u},
            (GameJobOrderHandle){0u, 0u},
            current_tick,
            threat_level,
            resume_when_clear,
            GAME_EMERGENCY_AUDIT_IDLE
        );
        return GAME_EMERGENCY_RESULT_OK;
    }

    GameJobOrder current_order = {0};
    GameJobBoardResult status = game_job_board_status(board, worker->current_job, &current_order);
    if (status != GAME_JOB_BOARD_RESULT_OK) {
        worker->has_current_job = false;
        worker->current_job = (GameJobOrderHandle){0u, 0u};
        slot->in_use = false;
        return GAME_EMERGENCY_RESULT_INVALID_HANDLE;
    }

    if ((current_order.required_role_flags & GAME_WORKER_ROLE_EMERGENCY) != 0u) {
        game_emergency_record_audit(
            audit_log,
            worker->id,
            worker->current_job,
            (GameJobOrderHandle){0u, 0u},
            current_tick,
            threat_level,
            resume_when_clear,
            GAME_EMERGENCY_AUDIT_IDLE
        );
        return GAME_EMERGENCY_RESULT_OK;
    }

    if (current_order.state != GAME_JOB_BOARD_STATE_RESERVED && current_order.state != GAME_JOB_BOARD_STATE_IN_PROGRESS) {
        game_emergency_record_audit(
            audit_log,
            worker->id,
            worker->current_job,
            (GameJobOrderHandle){0u, 0u},
            current_tick,
            threat_level,
            resume_when_clear,
            GAME_EMERGENCY_AUDIT_IDLE
        );
        return GAME_EMERGENCY_RESULT_INVALID_STATE;
    }

    GameJobBoardResult transition = game_job_board_transition(board, worker->current_job, GAME_JOB_BOARD_STATE_STALLED);
    if (transition != GAME_JOB_BOARD_RESULT_OK) {
        return game_emergency_map_board(transition);
    }

    GameJobBoardResult release = game_job_board_release_reservation(board, worker->current_job);
    if (release != GAME_JOB_BOARD_RESULT_OK && release != GAME_JOB_BOARD_RESULT_INVALID_STATE) {
        return game_emergency_map_board(release);
    }

    if (active_haul_job.slot != 0u || active_haul_job.version != 0u) {
        GameHaulJobResult halt = game_haul_job_abort(
            haul_system,
            inventory,
            board,
            active_haul_job,
            current_tick,
            NULL,
            NULL
        );
        GameEmergencyResult mapped = game_emergency_map_haul(halt);
        if (mapped != GAME_EMERGENCY_RESULT_OK) {
            return mapped;
        }
    }

    GameJobOrderHandle emergency_order = {0u, 0u};
    GameJobBoardResult create = game_job_board_create_order(
        board,
        current_tick,
        current_order.target,
        emergency_role_flags,
        emergency_required_resource_flags,
        emergency_duration_min_ticks,
        emergency_duration_max_ticks,
        &emergency_order
    );
    if (create != GAME_JOB_BOARD_RESULT_OK) {
        return game_emergency_map_board(create);
    }

    GameJobBoardResult reserve = game_job_board_reserve(board, emergency_order, current_tick, worker->id);
    if (reserve != GAME_JOB_BOARD_RESULT_OK) {
        (void)game_job_board_transition(board, emergency_order, GAME_JOB_BOARD_STATE_ABORTED);
        return game_emergency_map_board(reserve);
    }

    GameJobBoardResult start = game_job_board_transition(board, emergency_order, GAME_JOB_BOARD_STATE_IN_PROGRESS);
    if (start != GAME_JOB_BOARD_RESULT_OK) {
        (void)game_job_board_transition(board, emergency_order, GAME_JOB_BOARD_STATE_ABORTED);
        return game_emergency_map_board(start);
    }

    slot->active = true;
    slot->resume_on_clear = resume_when_clear;
    slot->routine_order = worker->current_job;
    slot->emergency_order = emergency_order;
    slot->interruption_tick = current_tick;
    slot->last_threat_level = threat_level;
    slot->worker = worker->id;

    worker->current_job = emergency_order;
    worker->has_current_job = true;

    game_emergency_record_audit(
        audit_log,
        worker->id,
        slot->routine_order,
        slot->emergency_order,
        current_tick,
        threat_level,
        resume_when_clear,
        GAME_EMERGENCY_AUDIT_EMERGENCY_CREATED
    );

    return GAME_EMERGENCY_RESULT_OK;
}
