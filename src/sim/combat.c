#include "sim/combat.h"

#include <string.h>

#include "ecs/entity.h"
#include "event/event_queue.h"
#include "world/hex.h"

typedef struct {
    uint32_t ability_id;
    uint32_t target_index;
    int32_t target_distance;
    int32_t damage;
    uint64_t cooldown_next_tick;
    uint64_t resolved_tick;
} GameCombatAbilityUsedPayload;

static size_t game_combat_slot_find(
    const GameCombatCooldownTable *table,
    GameEntityId actor,
    uint32_t ability_id,
    size_t *out_index
) {
    if (!table || !table->slots) {
        return (size_t)-1;
    }

    for (size_t i = 0u; i < table->count; ++i) {
        const GameCombatCooldownSlot *slot = &table->slots[i];
        if (slot->in_use && slot->actor.index == actor.index && slot->actor.generation == actor.generation
            && slot->ability_id == ability_id) {
            if (out_index) {
                *out_index = i;
            }
            return i;
        }
    }
    return (size_t)-1;
}

GameCombatResult game_combat_init_catalog(GameCombatAbilityCatalog *catalog, GameCombatAbility *entries, size_t capacity) {
    if (!catalog || (!entries && capacity > 0u)) {
        return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
    }

    catalog->entries = entries;
    catalog->capacity = capacity;
    catalog->count = 0u;
    return GAME_COMBAT_RESULT_OK;
}

GameCombatResult game_combat_catalog_set(
    GameCombatAbilityCatalog *catalog,
    uint32_t ability_id,
    uint32_t range_tiles,
    uint64_t cooldown_ticks,
    int32_t base_damage
) {
    if (!catalog || !catalog->entries || catalog->capacity == 0u) {
        return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
    }

    if (range_tiles == 0u) {
        range_tiles = 1u;
    }

    for (size_t i = 0u; i < catalog->count; ++i) {
        if (catalog->entries[i].ability_id == ability_id) {
            catalog->entries[i].range_tiles = range_tiles;
            catalog->entries[i].cooldown_ticks = cooldown_ticks;
            catalog->entries[i].base_damage = base_damage;
            return GAME_COMBAT_RESULT_OK;
        }
    }

    if (catalog->count >= catalog->capacity) {
        return GAME_COMBAT_RESULT_BUFFER_TOO_SMALL;
    }

    catalog->entries[catalog->count++] = (GameCombatAbility){
        .ability_id = ability_id,
        .range_tiles = range_tiles,
        .cooldown_ticks = cooldown_ticks,
        .base_damage = base_damage,
    };

    return GAME_COMBAT_RESULT_OK;
}

GameCombatResult game_combat_catalog_find(
    const GameCombatAbilityCatalog *catalog,
    uint32_t ability_id,
    GameCombatAbility *out_ability
) {
    if (!catalog || !catalog->entries) {
        return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < catalog->count; ++i) {
        if (catalog->entries[i].ability_id == ability_id) {
            if (out_ability) {
                *out_ability = catalog->entries[i];
            }
            return GAME_COMBAT_RESULT_OK;
        }
    }
    return GAME_COMBAT_RESULT_UNKNOWN_ABILITY;
}

GameCombatResult game_combat_cooldown_init(GameCombatCooldownTable *table, GameCombatCooldownSlot *slots, size_t capacity) {
    if (!table || (!slots && capacity > 0u)) {
        return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
    }

    table->slots = slots;
    table->capacity = capacity;
    table->count = 0u;
    return GAME_COMBAT_RESULT_OK;
}

GameCombatResult game_combat_get_or_create_cooldown(
    GameCombatCooldownTable *table,
    GameEntityId actor,
    uint32_t ability_id,
    GameCombatCooldownHandle *out_handle
) {
    if (!table || !out_handle || !game_entity_id_is_valid(actor)) {
        return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
    }

    size_t existing = game_combat_slot_find(table, actor, ability_id, NULL);
    if (existing != (size_t)-1) {
        *out_handle = table->slots[existing].handle;
        return GAME_COMBAT_RESULT_OK;
    }

    if (!table->slots || table->capacity == 0u || table->count >= table->capacity) {
        return GAME_COMBAT_RESULT_BUFFER_TOO_SMALL;
    }

    GameCombatCooldownSlot *slot = &table->slots[table->count];
    *slot = (GameCombatCooldownSlot){
        .handle = {(uint32_t)table->count + 1u, 1u},
        .generation = 1u,
        .actor = actor,
        .ability_id = ability_id,
        .last_used_tick = UINT64_MAX,
        .in_use = true,
    };
    *out_handle = slot->handle;
    ++table->count;
    return GAME_COMBAT_RESULT_OK;
}

GameCombatResult game_combat_clear_cooldowns(GameCombatCooldownTable *table) {
    if (!table) {
        return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
    }
    if (!table->slots) {
        table->count = 0u;
        return GAME_COMBAT_RESULT_OK;
    }
    table->count = 0u;
    return GAME_COMBAT_RESULT_OK;
}

static GameCombatResult game_combat_validate_command(const GameCombatAbilityCommand *command) {
    if (!command) {
        return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
    }

    if (!game_entity_id_is_valid(command->actor) || !game_entity_id_is_valid(command->target)) {
        return GAME_COMBAT_RESULT_INVALID_HANDLE;
    }
    return GAME_COMBAT_RESULT_OK;
}

static GameCombatResult game_combat_emit_ability_used_event(
    const GameCombatAbilityCommand *command,
    const GameCombatAbility *ability,
    uint64_t current_tick,
    uint64_t cooldown_next_tick,
    uint64_t *out_event_sequence,
    GameEventQueue *event_queue,
    GameEventLog *event_log
) {
    if (!event_log && !event_queue) {
        if (out_event_sequence) {
            *out_event_sequence = GAME_EVENT_LOG_INVALID_PARENT_ID;
        }
        return GAME_COMBAT_RESULT_OK;
    }

    GameCombatAbilityUsedPayload payload = {
        .ability_id = ability->ability_id,
        .target_index = command->target.index,
        .target_distance = game_hex_axial_distance(command->actor_position, command->target_position),
        .damage = ability->base_damage,
        .cooldown_next_tick = cooldown_next_tick,
        .resolved_tick = current_tick,
    };

    uint64_t ability_seq = GAME_EVENT_LOG_INVALID_PARENT_ID;
    if (out_event_sequence) {
        *out_event_sequence = GAME_EVENT_LOG_INVALID_PARENT_ID;
    }
    if (event_log) {
        GameEventLogResult append_result =
            game_event_log_append(event_log, current_tick, GAME_EVENT_TYPE_ABILITY_USED, command->actor, GAME_EVENT_LOG_INVALID_PARENT_ID, &ability_seq);
        if (append_result != GAME_EVENT_LOG_RESULT_OK) {
            return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
        }
        if (out_event_sequence) {
            *out_event_sequence = ability_seq;
        }
    }

    if (event_queue) {
        GameEvent event = {
            .type = GAME_EVENT_TYPE_ABILITY_USED,
            .tick = current_tick,
            .source = command->actor,
            .payload_size = sizeof(GameCombatAbilityUsedPayload),
        };
        memcpy(event.payload.bytes, &payload, sizeof(GameCombatAbilityUsedPayload));
        if (game_event_queue_push(event_queue, &event) != GAME_EVENT_QUEUE_RESULT_OK) {
            return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
        }
    }

    return GAME_COMBAT_RESULT_OK;
}

GameCombatResult game_combat_resolve_ability(
    const GameCombatAbilityCatalog *catalog,
    GameCombatCooldownTable *cooldowns,
    const GameCombatAbilityCommand *command,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    GameCombatResultData *out_result
) {
    if (!catalog || !cooldowns || !command) {
        return GAME_COMBAT_RESULT_INVALID_ARGUMENT;
    }

    if (game_combat_validate_command(command) != GAME_COMBAT_RESULT_OK) {
        return GAME_COMBAT_RESULT_INVALID_HANDLE;
    }

    GameCombatAbility ability = {0};
    if (game_combat_catalog_find(catalog, command->ability_id, &ability) != GAME_COMBAT_RESULT_OK) {
        return GAME_COMBAT_RESULT_UNKNOWN_ABILITY;
    }

    int32_t target_distance = game_hex_axial_distance(command->actor_position, command->target_position);
    if (target_distance > (int32_t)ability.range_tiles) {
        return GAME_COMBAT_RESULT_OUT_OF_RANGE;
    }

    GameCombatCooldownHandle handle = {0u, 0u};
    GameCombatResult cooldown_result =
        game_combat_get_or_create_cooldown(cooldowns, command->actor, command->ability_id, &handle);
    if (cooldown_result != GAME_COMBAT_RESULT_OK) {
        return cooldown_result;
    }

    size_t index = (size_t)-1;
    if (game_combat_slot_find(cooldowns, command->actor, command->ability_id, &index) == (size_t)-1) {
        return GAME_COMBAT_RESULT_INVALID_HANDLE;
    }
    GameCombatCooldownSlot *slot = &cooldowns->slots[index];

    uint64_t cooldown_next_tick = 0u;
    if (slot->last_used_tick != UINT64_MAX) {
        cooldown_next_tick = slot->last_used_tick + ability.cooldown_ticks;
    }
    if (slot->last_used_tick != UINT64_MAX && current_tick < cooldown_next_tick) {
        return GAME_COMBAT_RESULT_COOLDOWN;
    }

    uint64_t ability_event_sequence = GAME_EVENT_LOG_INVALID_PARENT_ID;
    GameCombatResult emit = game_combat_emit_ability_used_event(
        command,
        &ability,
        current_tick,
        cooldown_next_tick,
        &ability_event_sequence,
        event_queue,
        event_log
    );
    if (emit != GAME_COMBAT_RESULT_OK) {
        return emit;
    }

    slot->last_used_tick = current_tick;
    cooldown_next_tick = slot->last_used_tick + ability.cooldown_ticks;

    if (out_result) {
        *out_result = (GameCombatResultData){
            .ability_id = command->ability_id,
            .resolved_tick = current_tick,
            .cooldown_next_tick = cooldown_next_tick,
            .target_distance = target_distance,
            .ability_event_sequence = ability_event_sequence,
            .damage = ability.base_damage,
            .cooldown_handle = handle,
        };
    }
    return GAME_COMBAT_RESULT_OK;
}
