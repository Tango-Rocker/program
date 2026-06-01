#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "colony/haul_job.h"
#include "colony/inventory.h"
#include "colony/job_board.h"
#include "ecs/entity.h"
#include "event/event_log.h"
#include "nav/path_service.h"
#include "world/hex.h"
#include "world/structure.h"
#include "world/world_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_CONSTRUCTION_MAX_REQUIREMENTS 4u

typedef enum {
    GAME_CONSTRUCTION_RESULT_OK = 0,
    GAME_CONSTRUCTION_RESULT_INVALID_ARGUMENT = 1,
    GAME_CONSTRUCTION_RESULT_INVALID_HANDLE = 2,
    GAME_CONSTRUCTION_RESULT_NO_SLOT = 3,
    GAME_CONSTRUCTION_RESULT_JOB_BOARD_ERROR = 4,
    GAME_CONSTRUCTION_RESULT_INVENTORY_ERROR = 5,
    GAME_CONSTRUCTION_RESULT_HAUL_ERROR = 6,
    GAME_CONSTRUCTION_RESULT_STRUCTURE_ERROR = 7,
    GAME_CONSTRUCTION_RESULT_INVALID_STATE = 8,
} GameConstructionResult;

typedef enum {
    GAME_CONSTRUCTION_STATE_PLANNING = 0u,
    GAME_CONSTRUCTION_STATE_DELIVERING = 1u,
    GAME_CONSTRUCTION_STATE_BUILDING = 2u,
    GAME_CONSTRUCTION_STATE_DONE = 3u,
    GAME_CONSTRUCTION_STATE_ABORTED = 4u,
    GAME_CONSTRUCTION_STATE_STALLED = 5u,
} GameConstructionWorksiteState;

typedef enum {
    GAME_CONSTRUCTION_AUDIT_CREATED = 0u,
    GAME_CONSTRUCTION_AUDIT_DELIVERY_STARTED = 1u,
    GAME_CONSTRUCTION_AUDIT_DELIVERY_COMPLETED = 2u,
    GAME_CONSTRUCTION_AUDIT_BUILDING = 3u,
    GAME_CONSTRUCTION_AUDIT_COMPLETED = 4u,
    GAME_CONSTRUCTION_AUDIT_STALLED = 5u,
    GAME_CONSTRUCTION_AUDIT_ABORTED = 6u,
} GameConstructionAuditReason;

typedef struct {
    uint32_t slot;
    uint32_t version;
} GameConstructionWorksiteHandle;

typedef struct {
    uint32_t resource_id;
    uint32_t required_amount;
    uint32_t delivered_amount;
} GameConstructionResourceRequirement;

typedef struct {
    GameConstructionWorksiteHandle handle;
    uint32_t stable_id;
    bool in_use;
    GameConstructionWorksiteState state;
    GameHexAxial target_tile;
    uint32_t structure_definition_id;
    GameEntityId assigned_worker;
    GameHexAxial haul_start_tile;
    GameHexAxial haul_end_tile;
    GameInventoryOwner resource_source_owner;
    GameInventoryOwner resource_destination_owner;
    uint32_t required_work_ticks;
    uint32_t progress_ticks;
    uint32_t active_requirement_index;
    uint32_t worker_capacity;
    GamePathRequestHandle haul_route_request;
    GameJobOrderHandle work_order;
    GameHaulJobHandle active_haul;
    GameStructurePlacementHandle structure_placement;
    bool has_structure_placement;
    uint32_t requirement_count;
    GameConstructionResourceRequirement requirements[GAME_CONSTRUCTION_MAX_REQUIREMENTS];
} GameConstructionWorksite;

typedef struct {
    size_t capacity;
    uint32_t next_stable_id;
    GameConstructionWorksite *sites;
} GameConstructionSystem;

typedef struct {
    GameConstructionWorksiteHandle worksite;
    uint64_t tick;
    GameConstructionWorksiteState state;
    GameConstructionAuditReason reason;
    uint32_t requirement_index;
    uint32_t requirement_delivered;
    uint32_t progress_ticks;
} GameConstructionAuditEntry;

typedef struct {
    GameConstructionAuditEntry *entries;
    size_t capacity;
    size_t count;
} GameConstructionAuditLog;

typedef struct {
    GameHexAxial target_tile;
    uint32_t structure_definition_id;
    GameEntityId assigned_worker;
    GameInventoryOwner resource_source_owner;
    GameInventoryOwner resource_destination_owner;
    uint32_t required_work_ticks;
    uint32_t worker_capacity;
    uint32_t required_role_flags;
    uint32_t required_resource_flags;
    uint32_t duration_min_ticks;
    uint32_t duration_max_ticks;
    GamePathRequestHandle haul_route_request;
    const GameConstructionResourceRequirement *requirements;
    size_t requirement_count;
} GameConstructionCreateInfo;

void game_construction_audit_init(GameConstructionAuditLog *log, GameConstructionAuditEntry *entries, size_t capacity);

GameConstructionResult game_construction_init(
    GameConstructionSystem *system,
    GameConstructionWorksite *sites,
    size_t capacity,
    uint32_t next_stable_id
);

GameConstructionResult game_construction_create(
    GameConstructionSystem *system,
    GameJobBoard *board,
    GameInventory *inventory,
    uint64_t current_tick,
    const GameConstructionCreateInfo *info,
    GameConstructionWorksiteHandle *out_handle
);

GameConstructionResult game_construction_status(
    const GameConstructionSystem *system,
    GameConstructionWorksiteHandle handle,
    GameConstructionWorksite *out_worksite
);

GameConstructionResult game_construction_set_worker(
    GameConstructionSystem *system,
    GameConstructionWorksiteHandle handle,
    GameEntityId worker
);

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
);

GameConstructionResult game_construction_abort(
    GameConstructionSystem *system,
    GameJobBoard *board,
    GameHaulJobSystem *haul_system,
    GameInventory *inventory,
    GameConstructionWorksiteHandle handle,
    uint64_t current_tick,
    GameEventLog *event_log,
    GameConstructionAuditLog *audit_log
);

GameConstructionResult game_construction_clear(
    GameConstructionSystem *system,
    GameJobBoard *board,
    GameHaulJobSystem *haul_system,
    GameConstructionWorksiteHandle handle
);

uint32_t game_construction_progress(const GameConstructionSystem *system, GameConstructionWorksiteHandle handle);
uint32_t game_construction_delivered_total(const GameConstructionWorksite *worksite);
uint32_t game_construction_required_total(const GameConstructionWorksite *worksite);

#ifdef __cplusplus
}
#endif
