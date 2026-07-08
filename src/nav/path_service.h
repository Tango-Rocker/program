#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "nav/pathfind.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_PATH_SERVICE_STATE_PENDING = 0,
    GAME_PATH_SERVICE_STATE_RESOLVED = 1,
    GAME_PATH_SERVICE_STATE_FAILED = 2,
    GAME_PATH_SERVICE_STATE_CANCELLED = 3,
    GAME_PATH_SERVICE_STATE_EXPIRED = 4,
} GamePathServiceState;

typedef struct GamePathRequestHandle {
    uint32_t slot;
    uint32_t version;
} GamePathRequestHandle;

typedef struct GamePathServiceRequestSlot {
    bool in_use;
    uint32_t slot_id;
    uint32_t version;
    GamePathRequestHandle handle;
    GamePathServiceState state;
    uint64_t requested_at_tick;
    uint64_t expires_at_tick;
    uint64_t result_version;
    uint32_t request_budget;
    GameHexAxial start;
    GameHexAxial goal;
    const GamePathCostMap *cost_map;
    GamePathQueryScratch *scratch;
    GameHexAxial *result_path;
    size_t result_path_capacity;
    size_t result_path_length;
    GamePathFindResult last_query_result;
    bool has_result;
} GamePathServiceRequestSlot;

typedef struct GamePathService {
    size_t capacity;
    uint64_t ttl_ticks;
    GamePathServiceRequestSlot *slots;
    uint64_t current_tick;
} GamePathService;

typedef enum {
    GAME_PATH_SERVICE_RESULT_OK = 0,
    GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT = 1,
    GAME_PATH_SERVICE_RESULT_INVALID_HANDLE = 2,
    GAME_PATH_SERVICE_RESULT_NO_SLOT = 3,
    GAME_PATH_SERVICE_RESULT_PATH_TOO_SMALL = 4,
} GamePathServiceResult;

typedef struct GamePathServiceStatus {
    GamePathServiceState state;
    uint64_t requested_at_tick;
    uint64_t expires_at_tick;
    uint64_t result_version;
    size_t result_path_length;
    uint32_t request_budget;
    GamePathFindResult last_query_result;
    bool has_result_path;
} GamePathServiceStatus;

GamePathServiceResult game_path_service_init(
    GamePathService *service,
    GamePathServiceRequestSlot *slots,
    size_t slot_count,
    uint64_t ttl_ticks
);

GamePathServiceResult game_path_service_submit(
    GamePathService *service,
    uint64_t current_tick,
    GameHexAxial start,
    GameHexAxial goal,
    const GamePathCostMap *cost_map,
    GamePathQueryScratch *scratch,
    uint32_t request_budget,
    GameHexAxial *result_path_buffer,
    size_t result_path_capacity,
    GamePathRequestHandle *out_handle
);

GamePathServiceResult game_path_service_cancel(GamePathService *service, GamePathRequestHandle handle);
GamePathServiceResult game_path_service_status(
    const GamePathService *service,
    GamePathRequestHandle handle,
    GamePathServiceStatus *out_status
);
GamePathServiceResult game_path_service_retrieve(
    const GamePathService *service,
    GamePathRequestHandle handle,
    GameHexAxial *out_path,
    size_t out_capacity,
    size_t *out_length,
    uint64_t *out_result_version
);

GamePathServiceResult game_path_service_advance_tick(GamePathService *service, uint64_t current_tick, uint32_t tick_budget);

#ifdef __cplusplus
}
#endif
