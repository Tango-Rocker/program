#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_JOB_BOARD_STATE_OPEN = 0,
    GAME_JOB_BOARD_STATE_RESERVED = 1,
    GAME_JOB_BOARD_STATE_IN_PROGRESS = 2,
    GAME_JOB_BOARD_STATE_STALLED = 3,
    GAME_JOB_BOARD_STATE_DONE = 4,
    GAME_JOB_BOARD_STATE_ABORTED = 5,
} GameJobBoardOrderState;

typedef enum {
    GAME_JOB_BOARD_RESULT_OK = 0,
    GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT = 1,
    GAME_JOB_BOARD_RESULT_INVALID_HANDLE = 2,
    GAME_JOB_BOARD_RESULT_NO_SLOT = 3,
    GAME_JOB_BOARD_RESULT_ALREADY_RESERVED = 4,
    GAME_JOB_BOARD_RESULT_INVALID_STATE = 5,
} GameJobBoardResult;

typedef struct GameJobOrderHandle {
    uint32_t slot;
    uint32_t version;
} GameJobOrderHandle;

typedef struct GameJobOrder {
    uint32_t stable_id;
    GameHexAxial target;
    uint32_t required_role_flags;
    uint32_t required_resource_flags;
    uint32_t duration_min_ticks;
    uint32_t duration_max_ticks;
    GameJobBoardOrderState state;
    GameEntityId reserved_by;
    uint64_t expires_at_tick;
    bool reserved;
} GameJobOrder;

typedef struct GameJobBoardOrderSlot {
    bool in_use;
    uint32_t version;
    GameJobOrderHandle handle;
    uint32_t stable_id;
    GameJobOrder order;
    uint64_t reserved_at_tick;
    uint64_t requested_at_tick;
} GameJobBoardOrderSlot;

typedef struct GameJobBoard {
    size_t capacity;
    uint64_t reservation_ttl_ticks;
    uint64_t current_tick;
    uint32_t next_stable_id;
    GameJobBoardOrderSlot *slots;
} GameJobBoard;

GameJobBoardResult game_job_board_init(
    GameJobBoard *board,
    GameJobBoardOrderSlot *slots,
    size_t slot_count,
    uint64_t reservation_ttl_ticks
);

GameJobBoardResult game_job_board_create_order(
    GameJobBoard *board,
    uint64_t current_tick,
    GameHexAxial target,
    uint32_t required_role_flags,
    uint32_t required_resource_flags,
    uint32_t duration_min_ticks,
    uint32_t duration_max_ticks,
    GameJobOrderHandle *out_handle
);

GameJobBoardResult game_job_board_reserve(
    GameJobBoard *board,
    GameJobOrderHandle handle,
    uint64_t current_tick,
    GameEntityId worker
);

GameJobBoardResult game_job_board_transition(
    GameJobBoard *board,
    GameJobOrderHandle handle,
    GameJobBoardOrderState next_state
);
GameJobBoardResult game_job_board_release_reservation(GameJobBoard *board, GameJobOrderHandle handle);

GameJobBoardResult game_job_board_advance_tick(GameJobBoard *board, uint64_t current_tick);

GameJobBoardResult game_job_board_status(
    const GameJobBoard *board,
    GameJobOrderHandle handle,
    GameJobOrder *out_order
);

size_t game_job_board_capacity(const GameJobBoard *board);

#ifdef __cplusplus
}
#endif
