#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_SCHEDULER_MAX_PAYLOAD_BYTES 32u

typedef enum {
    GAME_SCHEDULER_RESULT_OK = 0,
    GAME_SCHEDULER_RESULT_INVALID_ARGUMENT = 1,
    GAME_SCHEDULER_RESULT_FULL = 2,
    GAME_SCHEDULER_RESULT_INVALID_HANDLE = 3,
    GAME_SCHEDULER_RESULT_ALREADY_CANCELLED = 4,
    GAME_SCHEDULER_RESULT_OUTPUT_TOO_SMALL = 5,
} GameSchedulerResult;

typedef struct {
    uint32_t slot;
    uint32_t generation;
} GameSchedulerHandle;

typedef struct {
    bool occupied;
    bool cancelled;
    bool fired;
    uint32_t generation;
    uint64_t due_tick;
    uint64_t sequence;
    uint8_t type;
    uint8_t payload_size;
    uint16_t reserved;
    GameEntityId source;
    uint8_t payload[GAME_SCHEDULER_MAX_PAYLOAD_BYTES];
} GameSchedulerEntry;

typedef struct {
    size_t capacity;
    size_t active_count;
    uint64_t next_sequence;
    GameSchedulerEntry *entries;
} GameScheduler;

GameSchedulerResult game_scheduler_init(GameScheduler *scheduler, size_t capacity);
void game_scheduler_destroy(GameScheduler *scheduler);
void game_scheduler_destroy_contents(GameScheduler *scheduler);

GameSchedulerResult game_scheduler_schedule(
    GameScheduler *scheduler,
    uint64_t due_tick,
    uint8_t type,
    GameEntityId source,
    const void *payload,
    uint8_t payload_size,
    GameSchedulerHandle *out_handle
);

GameSchedulerResult game_scheduler_cancel(GameScheduler *scheduler, GameSchedulerHandle handle);

typedef struct {
    GameSchedulerHandle handle;
    uint64_t due_tick;
    uint64_t sequence;
    uint8_t type;
    GameEntityId source;
    uint8_t payload_size;
    uint8_t payload[GAME_SCHEDULER_MAX_PAYLOAD_BYTES];
} GameScheduledEvent;

GameSchedulerResult game_scheduler_dispatch(
    GameScheduler *scheduler,
    uint64_t current_tick,
    GameScheduledEvent *out_events,
    size_t out_events_capacity,
    size_t *out_dispatched_count
);

size_t game_scheduler_active_count(const GameScheduler *scheduler);

size_t game_scheduler_serialize(
    const GameScheduler *scheduler,
    GameSchedulerEntry *out_entries,
    size_t out_capacity
);

#ifdef __cplusplus
}
#endif
