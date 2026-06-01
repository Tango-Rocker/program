#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "event/event_queue.h"
#include "event/event.h"
#include "event/event_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_STATUS_EFFECT_RESULT_OK = 0,
    GAME_STATUS_EFFECT_RESULT_INVALID_ARGUMENT = 1,
    GAME_STATUS_EFFECT_RESULT_BUFFER_TOO_SMALL = 2,
    GAME_STATUS_EFFECT_RESULT_STALE_HANDLE = 3,
    GAME_STATUS_EFFECT_RESULT_INVALID_HANDLE = 4,
} GameStatusEffectResult;

typedef enum {
    GAME_STATUS_EFFECT_TYPE_NONE = 0u,
    GAME_STATUS_EFFECT_TYPE_POISON = 1u,
    GAME_STATUS_EFFECT_TYPE_BURN = 2u,
} GameStatusEffectType;

typedef struct {
    uint32_t slot;
    uint32_t generation;
} GameStatusEffectHandle;

typedef struct {
    uint32_t effect_id;
    int32_t magnitude;
    uint32_t duration_ticks;
    int32_t target_q;
    int32_t target_r;
} GameStatusEffectPayload;

typedef struct {
    GameStatusEffectHandle handle;
    uint32_t generation;
    bool in_use;
    GameEntityId target;
    GameEntityId source;
    uint32_t effect_id;
    int32_t magnitude;
    uint32_t duration_ticks;
    uint64_t applied_tick;
    int32_t target_q;
    int32_t target_r;
} GameStatusEffectInstance;

typedef struct {
    size_t capacity;
    size_t count;
    GameStatusEffectInstance *slots;
} GameStatusEffectTable;

GameStatusEffectResult game_status_effect_init(
    GameStatusEffectTable *table,
    GameStatusEffectInstance *slots,
    size_t capacity
);
void game_status_effect_clear(GameStatusEffectTable *table);
GameStatusEffectResult game_status_effect_apply(
    GameStatusEffectTable *table,
    const GameEntityRegistry *registry,
    const GameEntityId target,
    const GameEntityId source,
    uint32_t effect_id,
    int32_t magnitude,
    uint32_t duration_ticks,
    int32_t target_q,
    int32_t target_r,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    uint64_t parent_event_sequence,
    GameStatusEffectHandle *out_handle
);
GameStatusEffectResult game_status_effect_advance(
    GameStatusEffectTable *table,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log
);
size_t game_status_effect_count(const GameStatusEffectTable *table);
GameStatusEffectResult game_status_effect_get(
    const GameStatusEffectTable *table,
    GameStatusEffectHandle handle,
    GameStatusEffectInstance *out_instance
);
bool game_status_effect_handle_is_valid(const GameStatusEffectTable *table, GameStatusEffectHandle handle);

#ifdef __cplusplus
}
#endif
