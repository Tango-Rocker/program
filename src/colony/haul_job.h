#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "colony/inventory.h"
#include "colony/job_board.h"
#include "event/event.h"
#include "event/event_log.h"
#include "nav/path_service.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_HAUL_JOB_RESULT_OK = 0,
    GAME_HAUL_JOB_RESULT_INVALID_ARGUMENT = 1,
    GAME_HAUL_JOB_RESULT_INVALID_HANDLE = 2,
    GAME_HAUL_JOB_RESULT_NO_SLOT = 3,
    GAME_HAUL_JOB_RESULT_INVENTORY_ERROR = 4,
    GAME_HAUL_JOB_RESULT_JOB_BOARD_ERROR = 5,
    GAME_HAUL_JOB_RESULT_INVALID_STATE = 6,
    GAME_HAUL_JOB_RESULT_STALE_PATH = 7,
    GAME_HAUL_JOB_RESULT_PATH_FAILED = 8,
} GameHaulJobResult;

typedef enum {
    GAME_HAUL_JOB_STATE_RESERVE_SOURCE = 0u,
    GAME_HAUL_JOB_STATE_PICKUP = 1u,
    GAME_HAUL_JOB_STATE_IN_TRANSIT = 2u,
    GAME_HAUL_JOB_STATE_DELIVERY = 3u,
    GAME_HAUL_JOB_STATE_STALLED = 4u,
    GAME_HAUL_JOB_STATE_DONE = 5u,
    GAME_HAUL_JOB_STATE_ABORTED = 6u,
} GameHaulJobState;

typedef enum {
    GAME_HAUL_JOB_AUDIT_CREATED = 0u,
    GAME_HAUL_JOB_AUDIT_SOURCE_RESERVED = 1u,
    GAME_HAUL_JOB_AUDIT_PICKUP = 2u,
    GAME_HAUL_JOB_AUDIT_IN_TRANSIT = 3u,
    GAME_HAUL_JOB_AUDIT_DELIVERED = 4u,
    GAME_HAUL_JOB_AUDIT_STALLED = 5u,
    GAME_HAUL_JOB_AUDIT_ABORTED = 6u,
} GameHaulJobAuditReason;

typedef enum {
    GAME_HAUL_JOB_PATH_STATUS_UNKNOWN = 0u,
    GAME_HAUL_JOB_PATH_STATUS_OK = 1u,
    GAME_HAUL_JOB_PATH_STATUS_MISSING = 2u,
    GAME_HAUL_JOB_PATH_STATUS_FAILED = 3u,
    GAME_HAUL_JOB_PATH_STATUS_STALE = 4u,
} GameHaulJobPathStatus;

typedef struct {
    uint32_t slot;
    uint32_t version;
} GameHaulJobHandle;

typedef struct {
    GameHaulJobHandle handle;
    uint32_t stable_id;
    bool in_use;
    GameEntityId worker;
    GameInventoryOwner source_owner;
    GameInventoryOwner destination_owner;
    uint32_t resource_id;
    uint32_t requested_amount;
    uint32_t delivered_amount;
    uint32_t worker_capacity;
    GameHexAxial source_tile;
    GameHexAxial destination_tile;
    GameHaulJobState state;
    GamePathRequestHandle route_request;
    GameInventoryReservationHandle source_reservation;
    GameJobOrderHandle work_order;
    uint64_t route_result_version;
} GameHaulJob;

typedef struct {
    size_t capacity;
    uint32_t next_stable_id;
    GameHaulJob *slots;
} GameHaulJobSystem;

typedef struct {
    GameHaulJobHandle job;
    uint64_t tick;
    GameEntityId worker;
    GameHaulJobState state;
    GameHaulJobAuditReason reason;
    uint32_t requested_amount;
    uint32_t delivered_amount;
} GameHaulJobAuditEntry;

typedef struct {
    GameHaulJobAuditEntry *entries;
    size_t capacity;
    size_t count;
} GameHaulJobAuditLog;

typedef struct {
    GameEntityId worker;
    GamePathRequestHandle route_request;
    GameInventoryOwner source_owner;
    GameInventoryOwner destination_owner;
    GameHexAxial source_tile;
    GameHexAxial destination_tile;
    uint32_t resource_id;
    uint32_t required_amount;
    uint32_t worker_capacity;
    uint32_t required_role_flags;
    uint32_t duration_min_ticks;
    uint32_t duration_max_ticks;
} GameHaulJobCreateInfo;

void game_haul_job_audit_init(GameHaulJobAuditLog *log, GameHaulJobAuditEntry *entries, size_t capacity);

GameHaulJobResult game_haul_job_init(GameHaulJobSystem *system, GameHaulJob *slots, size_t slot_count);
GameHaulJobResult game_haul_job_create(
    GameHaulJobSystem *system,
    GameInventory *inventory,
    GameJobBoard *board,
    uint64_t current_tick,
    const GameHaulJobCreateInfo *info,
    GameHaulJobHandle *out_handle
);

size_t game_haul_job_active_count(const GameHaulJobSystem *system);

GameHaulJobResult game_haul_job_status(
    const GameHaulJobSystem *system,
    GameHaulJobHandle handle,
    GameHaulJob *out_job
);

GameHaulJobResult game_haul_job_set_worker(GameHaulJobSystem *system, GameHaulJobHandle handle, GameEntityId worker);

GameHaulJobResult game_haul_job_assess_route(
    const GameHaulJobSystem *system,
    const GamePathService *path_service,
    GameHaulJobHandle handle,
    GameHaulJobPathStatus *out_status
);

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
);

GameHaulJobResult game_haul_job_abort(GameHaulJobSystem *system, GameInventory *inventory, GameJobBoard *board, GameHaulJobHandle handle, uint64_t current_tick, GameEventLog *event_log, GameHaulJobAuditLog *audit_log);

GameHaulJobResult game_haul_job_clear(GameHaulJobSystem *system, GameJobBoard *board, GameHaulJobHandle handle);

uint32_t game_haul_job_progress(const GameHaulJobSystem *system, GameHaulJobHandle handle);

#ifdef __cplusplus
}
#endif
