#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "colony/job_board.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Worker role flags used by the minimal deterministic selector.
 */
#define GAME_WORKER_ROLE_STANDARD ((uint32_t)0x00000001u)
#define GAME_WORKER_ROLE_EMERGENCY ((uint32_t)0x80000000u)

typedef struct GameWorkerState {
    GameEntityId id;
    uint32_t role_flags;
    uint32_t stamina;
    GameHexAxial position;
    uint32_t threat_level;
    bool has_current_job;
    GameJobOrderHandle current_job;
} GameWorkerState;

typedef enum {
    GAME_WORKER_AI_RESULT_OK = 0,
    GAME_WORKER_AI_RESULT_INVALID_ARGUMENT = 1,
    GAME_WORKER_AI_RESULT_NO_CANDIDATE = 2,
    GAME_WORKER_AI_RESULT_ALREADY_WORKING = 3,
    GAME_WORKER_AI_RESULT_AUDIT_FULL = 4,
    GAME_WORKER_AI_RESULT_RESERVATION_FAILED = 5,
} GameWorkerSelectionResult;

typedef enum {
    GAME_WORKER_AI_REASON_NONE = 0,
    GAME_WORKER_AI_REASON_SELECTED = 1,
    GAME_WORKER_AI_REASON_NO_MATCH = 2,
    GAME_WORKER_AI_REASON_THREAT_STALLED = 3,
    GAME_WORKER_AI_REASON_ALREADY_ASSIGNED = 4,
    GAME_WORKER_AI_REASON_STALE_RESERVATION_AVOIDED = 5,
    GAME_WORKER_AI_REASON_RESERVE_REFUSED = 6,
} GameWorkerSelectionReason;

typedef struct GameWorkerSelectionResultData {
    GameJobOrderHandle selected;
    GameWorkerSelectionReason reason;
    bool selected_any;
    uint32_t selected_role_match;
    uint32_t selected_urgency;
    uint32_t selected_distance;
    uint32_t selected_job_stable_id;
} GameWorkerSelectionResultData;

typedef struct GameWorkerSelectionAuditEntry {
    uint64_t tick;
    GameEntityId worker;
    GameJobOrderHandle job;
    GameWorkerSelectionReason reason;
    uint32_t job_stable_id;
    uint32_t distance;
    uint32_t urgency;
    uint32_t role_match;
} GameWorkerSelectionAuditEntry;

typedef struct GameWorkerSelectionAuditLog {
    GameWorkerSelectionAuditEntry *entries;
    size_t capacity;
    size_t count;
} GameWorkerSelectionAuditLog;

typedef struct GameWorkerSelectionConfig {
    uint64_t current_tick;
    uint32_t threat_stall_threshold;
    uint32_t emergency_role_mask;
} GameWorkerSelectionConfig;

void game_worker_ai_audit_init(GameWorkerSelectionAuditLog *log, GameWorkerSelectionAuditEntry *entries, size_t capacity);

GameWorkerSelectionResult game_worker_ai_select_job_for_worker(
    GameWorkerState *worker,
    GameJobBoard *board,
    const GameWorkerSelectionConfig *config,
    GameWorkerSelectionAuditLog *audit_log,
    GameWorkerSelectionResultData *out_result
);

#ifdef __cplusplus
}
#endif
