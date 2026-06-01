#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "event/event_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_PROJECTILE_RESULT_OK = 0,
    GAME_PROJECTILE_RESULT_INVALID_ARGUMENT = 1,
    GAME_PROJECTILE_RESULT_BUFFER_TOO_SMALL = 2,
    GAME_PROJECTILE_RESULT_STALE_HANDLE = 3,
    GAME_PROJECTILE_RESULT_OUT_OF_RANGE = 4,
    GAME_PROJECTILE_RESULT_FULL = 5,
} GameProjectileResult;

typedef struct {
    uint32_t slot;
    uint32_t generation;
} GameProjectileHandle;

typedef struct {
    uint32_t projectile_id;
    uint32_t source_index;
    uint32_t source_generation;
    int32_t target_q;
    int32_t target_r;
    int32_t damage;
    uint32_t ability_id;
} GameProjectileImpactPayload;

typedef struct {
    GameProjectileHandle handle;
    uint32_t projectile_id;
    uint32_t payload_id;
    uint32_t ability_id;
    uint64_t parent_event_sequence;
    GameEntityId source;
    GameEntityId target;
    int32_t source_q;
    int32_t source_r;
    int32_t target_q;
    int32_t target_r;
    uint32_t speed_per_tick;
    int32_t remaining_lifetime_ticks;
    int32_t current_q;
    int32_t current_r;
    int32_t target_distance;
    int32_t damage;
    bool in_flight;
} GameProjectileState;

typedef struct {
    GameProjectileState *slots;
    size_t capacity;
    size_t count;
    uint32_t next_id;
} GameProjectileTable;

typedef struct {
    GameEntityId source;
    GameEntityId target;
    int32_t source_q;
    int32_t source_r;
    int32_t target_q;
    int32_t target_r;
    int32_t damage;
    uint32_t speed_per_tick;
    int32_t remaining_lifetime_ticks;
    uint32_t payload_id;
    uint32_t ability_id;
    uint64_t parent_event_sequence;
} GameProjectileSpawnRequest;

GameProjectileResult game_projectile_init(
    GameProjectileTable *table,
    GameProjectileState *slots,
    size_t capacity
);
void game_projectile_clear(GameProjectileTable *table);
GameProjectileResult game_projectile_spawn(
    GameProjectileTable *table,
    const GameProjectileSpawnRequest *request,
    uint64_t current_tick,
    GameProjectileHandle *out_handle
);
GameProjectileResult game_projectile_advance_all(
    GameProjectileTable *table,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log
);
size_t game_projectile_count(const GameProjectileTable *table);
GameProjectileResult game_projectile_get(const GameProjectileTable *table, GameProjectileHandle handle, GameProjectileState *out_state);
bool game_projectile_handle_is_valid(const GameProjectileTable *table, GameProjectileHandle handle);

#ifdef __cplusplus
}
#endif
