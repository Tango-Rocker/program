#pragma once

#include <stddef.h>
#include <stdint.h>

#include "sim/scheduler.h"
#include "sim/sim_context.h"
#include "world/world_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_SNAPSHOT_VERSION 1u

typedef enum {
    GAME_SNAPSHOT_RESULT_OK = 0,
    GAME_SNAPSHOT_RESULT_INVALID_ARGUMENT = 1,
    GAME_SNAPSHOT_RESULT_UNSUPPORTED_VERSION = 2,
    GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL = 3,
    GAME_SNAPSHOT_RESULT_MALFORMED = 4,
    GAME_SNAPSHOT_RESULT_INIT_FAILED = 5,
} GameSnapshotResult;

typedef struct {
    size_t event_queue_capacity;
    size_t scheduler_capacity;
} GameSnapshotLoadConfig;

GameSnapshotResult game_snapshot_serialize(
    const GameSimContext *context,
    const GameWorldMap *world_map,
    const GameScheduler *scheduler,
    uint8_t *out_buffer,
    size_t out_capacity,
    size_t *out_size
);

GameSnapshotResult game_snapshot_deserialize(
    const uint8_t *buffer,
    size_t buffer_size,
    const GameSnapshotLoadConfig *config,
    GameSimContext *out_context,
    GameWorldMap *out_world_map,
    GameScheduler *out_scheduler
);

size_t game_snapshot_required_size(
    const GameSimContext *context,
    const GameWorldMap *world_map,
    const GameScheduler *scheduler
);

#ifdef __cplusplus
}
#endif
