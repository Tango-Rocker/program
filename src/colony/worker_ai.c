#include "colony/worker_ai.h"

#include <stdint.h>

static void game_worker_ai_set_defaults(
    const GameWorkerSelectionConfig *config,
    uint64_t *out_tick,
    uint32_t *out_threat_threshold,
    uint32_t *out_emergency_mask
) {
    *out_tick = config ? config->current_tick : 0u;
    *out_threat_threshold = config ? config->threat_stall_threshold : 1u;
    *out_emergency_mask = config ? config->emergency_role_mask : GAME_WORKER_ROLE_EMERGENCY;

    if (*out_threat_threshold == 0u) {
        *out_threat_threshold = 1u;
    }
    if (*out_emergency_mask == 0u) {
        *out_emergency_mask = GAME_WORKER_ROLE_EMERGENCY;
    }
}

void game_worker_ai_audit_init(GameWorkerSelectionAuditLog *log, GameWorkerSelectionAuditEntry *entries, size_t capacity) {
    if (!log) {
        return;
    }

    log->entries = entries;
    log->capacity = entries ? capacity : 0u;
    log->count = 0u;
}

static bool game_worker_ai_record_audit(
    GameWorkerSelectionAuditLog *log,
    GameWorkerState *worker,
    uint64_t current_tick,
    const GameWorkerSelectionResultData *result
) {
    if (!log || !log->entries || !worker || log->count >= log->capacity) {
        return false;
    }

    GameWorkerSelectionAuditEntry *entry = &log->entries[log->count++];
    entry->tick = current_tick;
    entry->worker = worker->id;
    entry->job = result->selected;
    entry->reason = result->reason;
    entry->job_stable_id = result->selected_job_stable_id;
    entry->distance = result->selected_distance;
    entry->urgency = result->selected_urgency;
    entry->role_match = result->selected_role_match;
    return true;
}

typedef struct {
    GameJobOrderHandle handle;
    uint32_t stable_id;
    uint32_t role_match;
    uint32_t urgency;
    uint32_t distance;
    bool emergency_job;
} GameWorkerCandidate;

static bool game_worker_ai_better_candidate(const GameWorkerCandidate *lhs, const GameWorkerCandidate *rhs) {
    if (lhs->emergency_job != rhs->emergency_job) {
        return !lhs->emergency_job;
    }
    if (lhs->role_match != rhs->role_match) {
        return lhs->role_match > rhs->role_match;
    }
    if (lhs->urgency != rhs->urgency) {
        return lhs->urgency > rhs->urgency;
    }
    if (lhs->distance != rhs->distance) {
        return lhs->distance < rhs->distance;
    }
    return lhs->stable_id < rhs->stable_id;
}

GameWorkerSelectionResult game_worker_ai_select_job_for_worker(
    GameWorkerState *worker,
    GameJobBoard *board,
    const GameWorkerSelectionConfig *config,
    GameWorkerSelectionAuditLog *audit_log,
    GameWorkerSelectionResultData *out_result
) {
    if (!worker || !board || !out_result || !board->slots || board->capacity == 0u) {
        return GAME_WORKER_AI_RESULT_INVALID_ARGUMENT;
    }

    uint64_t current_tick = 0u;
    uint32_t threat_threshold = 1u;
    uint32_t emergency_mask = GAME_WORKER_ROLE_EMERGENCY;
    game_worker_ai_set_defaults(config, &current_tick, &threat_threshold, &emergency_mask);

        *out_result = (GameWorkerSelectionResultData){
        .selected = {0u, 0u},
        .reason = GAME_WORKER_AI_REASON_NO_MATCH,
        .selected_any = false,
        .selected_role_match = 0u,
        .selected_urgency = UINT32_MAX,
        .selected_distance = UINT32_MAX,
        .selected_job_stable_id = UINT32_MAX,
    };

    if (!game_entity_id_is_valid(worker->id)) {
        return GAME_WORKER_AI_RESULT_INVALID_ARGUMENT;
    }

    if (worker->has_current_job) {
        GameJobOrder current_order = {0};
        GameJobBoardResult status = game_job_board_status(board, worker->current_job, &current_order);
        if (status == GAME_JOB_BOARD_RESULT_OK) {
            if (current_order.state == GAME_JOB_BOARD_STATE_IN_PROGRESS) {
                if (worker->threat_level >= threat_threshold
                    && (current_order.required_role_flags & emergency_mask) == 0u) {
                    (void)game_job_board_transition(
                        board,
                        worker->current_job,
                        GAME_JOB_BOARD_STATE_STALLED
                    );
                    out_result->reason = GAME_WORKER_AI_REASON_THREAT_STALLED;
                    game_worker_ai_record_audit(audit_log, worker, current_tick, out_result);
                    return GAME_WORKER_AI_RESULT_ALREADY_WORKING;
                }

                out_result->reason = GAME_WORKER_AI_REASON_ALREADY_ASSIGNED;
                out_result->selected_any = true;
                out_result->selected = worker->current_job;
                out_result->selected_role_match = 1u;
                out_result->selected_job_stable_id = current_order.stable_id;
                out_result->selected_urgency = current_order.duration_min_ticks;
                game_worker_ai_record_audit(audit_log, worker, current_tick, out_result);
                return GAME_WORKER_AI_RESULT_ALREADY_WORKING;
            }

            if (current_order.state == GAME_JOB_BOARD_STATE_STALLED) {
                out_result->reason = GAME_WORKER_AI_REASON_ALREADY_ASSIGNED;
                out_result->selected_any = true;
                out_result->selected = worker->current_job;
                out_result->selected_job_stable_id = current_order.stable_id;
                out_result->selected_urgency = current_order.duration_min_ticks;
                out_result->selected_role_match = 1u;
                game_worker_ai_record_audit(audit_log, worker, current_tick, out_result);
                return GAME_WORKER_AI_RESULT_ALREADY_WORKING;
            }

            if (current_order.state == GAME_JOB_BOARD_STATE_DONE || current_order.state == GAME_JOB_BOARD_STATE_ABORTED) {
                worker->has_current_job = false;
                worker->current_job = (GameJobOrderHandle){0u, 0u};
            }
        } else {
            worker->has_current_job = false;
            worker->current_job = (GameJobOrderHandle){0u, 0u};
        }
    }

    const bool emergency_only_mode = (worker->threat_level >= threat_threshold);
    bool has_reserved_conflict = false;
    bool seen_role_match = false;

    bool has_candidate = false;
    GameWorkerCandidate best = {
        .handle = {UINT32_MAX, UINT32_MAX},
        .stable_id = UINT32_MAX,
        .role_match = 0u,
        .urgency = UINT32_MAX,
        .distance = UINT32_MAX,
        .emergency_job = false,
    };

    for (size_t i = 0u; i < board->capacity; ++i) {
        GameJobBoardOrderSlot *slot = &board->slots[i];
        if (!slot->in_use) {
            continue;
        }

        if (slot->order.state != GAME_JOB_BOARD_STATE_OPEN) {
            if (slot->order.state == GAME_JOB_BOARD_STATE_RESERVED) {
                has_reserved_conflict = true;
            }
            continue;
        }

        if (emergency_only_mode && (slot->order.required_role_flags & emergency_mask) == 0u) {
            continue;
        }

        if ((slot->order.required_role_flags & worker->role_flags) != 0u) {
            seen_role_match = true;
        }

        GameWorkerCandidate current = {
            .handle = slot->handle,
            .stable_id = slot->order.stable_id,
            .role_match = ((slot->order.required_role_flags & worker->role_flags) != 0u) ? 1u : 0u,
            .urgency = slot->order.duration_min_ticks,
            .distance = (uint32_t)game_hex_axial_distance(worker->position, slot->order.target),
            .emergency_job = (slot->order.required_role_flags & emergency_mask) != 0u,
        };

        if (!has_candidate || game_worker_ai_better_candidate(&current, &best)) {
            best = current;
            has_candidate = true;
        }
    }

    if (!has_candidate) {
        if (emergency_only_mode) {
            out_result->reason = GAME_WORKER_AI_REASON_THREAT_STALLED;
        } else if (has_reserved_conflict) {
            out_result->reason = GAME_WORKER_AI_REASON_STALE_RESERVATION_AVOIDED;
        } else if (!seen_role_match) {
            out_result->reason = GAME_WORKER_AI_REASON_NO_MATCH;
        } else {
            out_result->reason = GAME_WORKER_AI_REASON_STALE_RESERVATION_AVOIDED;
        }
        game_worker_ai_record_audit(audit_log, worker, current_tick, out_result);
        return GAME_WORKER_AI_RESULT_NO_CANDIDATE;
    }

    GameJobOrder order = {0};
    if (game_job_board_status(board, best.handle, &order) != GAME_JOB_BOARD_RESULT_OK) {
        out_result->reason = GAME_WORKER_AI_REASON_RESERVE_REFUSED;
        return GAME_WORKER_AI_RESULT_NO_CANDIDATE;
    }

    if (order.state != GAME_JOB_BOARD_STATE_OPEN
        && !(order.state == GAME_JOB_BOARD_STATE_RESERVED
             && order.reserved_by.index == worker->id.index
             && order.reserved_by.generation == worker->id.generation)) {
        out_result->reason = GAME_WORKER_AI_REASON_STALE_RESERVATION_AVOIDED;
        game_worker_ai_record_audit(audit_log, worker, current_tick, out_result);
        return GAME_WORKER_AI_RESULT_NO_CANDIDATE;
    }

    GameJobBoardResult reserve_result = game_job_board_reserve(board, best.handle, current_tick, worker->id);
    if (reserve_result != GAME_JOB_BOARD_RESULT_OK) {
        if (reserve_result == GAME_JOB_BOARD_RESULT_ALREADY_RESERVED) {
            out_result->reason = GAME_WORKER_AI_REASON_RESERVE_REFUSED;
            return GAME_WORKER_AI_RESULT_RESERVATION_FAILED;
        }
        out_result->reason = GAME_WORKER_AI_REASON_RESERVE_REFUSED;
        return GAME_WORKER_AI_RESULT_NO_CANDIDATE;
    }

    worker->has_current_job = true;
    worker->current_job = best.handle;
    out_result->reason = GAME_WORKER_AI_REASON_SELECTED;
    out_result->selected_any = true;
    out_result->selected = best.handle;
    out_result->selected_role_match = best.role_match;
    out_result->selected_job_stable_id = best.stable_id;
    out_result->selected_distance = best.distance;
    out_result->selected_urgency = best.urgency;

    if (!game_worker_ai_record_audit(audit_log, worker, current_tick, out_result)) {
        return GAME_WORKER_AI_RESULT_AUDIT_FULL;
    }

    return GAME_WORKER_AI_RESULT_OK;
} 
