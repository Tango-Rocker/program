#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "colony/haul_job.h"
#include "colony/inventory.h"
#include "colony/job_board.h"
#include "colony/worker_ai.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_EMERGENCY_RESULT_OK = 0,
    GAME_EMERGENCY_RESULT_INVALID_ARGUMENT = 1,
    GAME_EMERGENCY_RESULT_INVALID_HANDLE = 2,
    GAME_EMERGENCY_RESULT_NO_SLOT = 3,
    GAME_EMERGENCY_RESULT_JOB_BOARD_ERROR = 4,
    GAME_EMERGENCY_RESULT_INVENTORY_ERROR = 5,
    GAME_EMERGENCY_RESULT_HAUL_ERROR = 6,
    GAME_EMERGENCY_RESULT_ALREADY_ACTIVE = 7,
    GAME_EMERGENCY_RESULT_INVALID_STATE = 8,
} GameEmergencyResult;

typedef enum {
    GAME_EMERGENCY_AUDIT_NONE = 0u,
    GAME_EMERGENCY_AUDIT_IDLE = 1u,
    GAME_EMERGENCY_AUDIT_ROUTINE_STALLED = 2u,
    GAME_EMERGENCY_AUDIT_EMERGENCY_CREATED = 3u,
    GAME_EMERGENCY_AUDIT_ROUTINE_RESUMED = 4u,
    GAME_EMERGENCY_AUDIT_ROUTINE_ABORTED = 5u,
    GAME_EMERGENCY_AUDIT_THREAT_CLEARED_NO_INTERRUPT = 6u,
} GameEmergencyAuditReason;

typedef struct {
    GameEntityId worker;
    bool in_use;
    bool active;
    bool resume_on_clear;
    GameJobOrderHandle routine_order;
    GameJobOrderHandle emergency_order;
    uint64_t interruption_tick;
    uint32_t last_threat_level;
} GameEmergencyInterruption;

typedef struct {
    size_t capacity;
    GameEmergencyInterruption *interruptions;
} GameEmergencySystem;

typedef struct {
    uint64_t tick;
    GameEntityId worker;
    GameJobOrderHandle routine_order;
    GameJobOrderHandle emergency_order;
    GameEmergencyAuditReason reason;
    uint32_t threat_level;
    bool resume_requested;
} GameEmergencyAuditEntry;

typedef struct {
    GameEmergencyAuditEntry *entries;
    size_t capacity;
    size_t count;
} GameEmergencyAuditLog;

void game_emergency_audit_init(GameEmergencyAuditLog *log, GameEmergencyAuditEntry *entries, size_t capacity);

GameEmergencyResult game_emergency_init(
    GameEmergencySystem *system,
    GameEmergencyInterruption *interruptions,
    size_t capacity
);

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
);

#ifdef __cplusplus
}
#endif
