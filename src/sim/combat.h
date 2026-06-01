#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "event/event.h"
#include "event/event_queue.h"
#include "event/event_log.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_COMBAT_RESULT_OK = 0,
    GAME_COMBAT_RESULT_INVALID_ARGUMENT = 1,
    GAME_COMBAT_RESULT_INVALID_HANDLE = 2,
    GAME_COMBAT_RESULT_OUT_OF_RANGE = 3,
    GAME_COMBAT_RESULT_COOLDOWN = 4,
    GAME_COMBAT_RESULT_DUPLICATE = 5,
    GAME_COMBAT_RESULT_UNKNOWN_ABILITY = 6,
    GAME_COMBAT_RESULT_OFF_MAP = 7,
    GAME_COMBAT_RESULT_BUFFER_TOO_SMALL = 8
} GameCombatResult;

typedef struct {
    uint32_t ability_id;
    uint32_t range_tiles;
    uint64_t cooldown_ticks;
    int32_t base_damage;
} GameCombatAbility;

typedef struct {
    GameCombatAbility *entries;
    size_t count;
    size_t capacity;
} GameCombatAbilityCatalog;

typedef struct {
    uint32_t index;
    uint32_t generation;
} GameCombatCooldownHandle;

typedef struct {
    GameCombatCooldownHandle handle;
    uint32_t generation;
    GameEntityId actor;
    uint32_t ability_id;
    uint64_t last_used_tick;
    bool in_use;
} GameCombatCooldownSlot;

typedef struct {
    GameCombatCooldownSlot *slots;
    size_t count;
    size_t capacity;
} GameCombatCooldownTable;

typedef struct {
    uint32_t ability_id;
    GameEntityId actor;
    GameEntityId target;
    GameHexAxial actor_position;
    GameHexAxial target_position;
} GameCombatAbilityCommand;

typedef struct {
    uint32_t ability_id;
    uint64_t resolved_tick;
    uint64_t cooldown_next_tick;
    int32_t target_distance;
    uint64_t ability_event_sequence;
    int32_t damage;
    GameCombatCooldownHandle cooldown_handle;
} GameCombatResultData;

GameCombatResult game_combat_init_catalog(GameCombatAbilityCatalog *catalog, GameCombatAbility *entries, size_t capacity);
GameCombatResult game_combat_catalog_set(
    GameCombatAbilityCatalog *catalog,
    uint32_t ability_id,
    uint32_t range_tiles,
    uint64_t cooldown_ticks,
    int32_t base_damage
);
GameCombatResult game_combat_catalog_find(
    const GameCombatAbilityCatalog *catalog,
    uint32_t ability_id,
    GameCombatAbility *out_ability
);
GameCombatResult game_combat_cooldown_init(GameCombatCooldownTable *table, GameCombatCooldownSlot *slots, size_t capacity);
GameCombatResult game_combat_get_or_create_cooldown(
    GameCombatCooldownTable *table,
    GameEntityId actor,
    uint32_t ability_id,
    GameCombatCooldownHandle *out_handle
);
GameCombatResult game_combat_clear_cooldowns(GameCombatCooldownTable *table);

GameCombatResult game_combat_resolve_ability(
    const GameCombatAbilityCatalog *catalog,
    GameCombatCooldownTable *cooldowns,
    const GameCombatAbilityCommand *command,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    GameCombatResultData *out_result
);

#ifdef __cplusplus
}
#endif
