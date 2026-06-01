#include "sim/status_effect.h"

#include <stdbool.h>
#include <string.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "event/event_queue.h"

static bool game_status_effect_table_is_valid(const GameStatusEffectTable *table) {
    return table && table->slots != NULL && table->capacity > 0u;
}

static bool game_status_effect_handle_matches(const GameStatusEffectInstance *slot, GameStatusEffectHandle handle) {
    return slot && slot->in_use && slot->handle.slot == handle.slot && slot->handle.generation == handle.generation;
}

GameStatusEffectResult game_status_effect_init(
    GameStatusEffectTable *table,
    GameStatusEffectInstance *slots,
    size_t capacity
) {
    if (!table || (!slots && capacity > 0u)) {
        return GAME_STATUS_EFFECT_RESULT_INVALID_ARGUMENT;
    }

    table->slots = slots;
    table->capacity = capacity;
    table->count = 0u;

    for (size_t i = 0u; i < capacity; ++i) {
        table->slots[i] = (GameStatusEffectInstance){
            .handle = {(uint32_t)i, 1u},
            .generation = 1u,
            .in_use = false,
        };
    }
    return GAME_STATUS_EFFECT_RESULT_OK;
}

void game_status_effect_clear(GameStatusEffectTable *table) {
    if (!table || !table->slots) {
        return;
    }
    for (size_t i = 0u; i < table->capacity; ++i) {
        ++table->slots[i].handle.generation;
        ++table->slots[i].generation;
        table->slots[i].in_use = false;
    }
    table->count = 0u;
}

static size_t game_status_effect_find_match(
    const GameStatusEffectTable *table,
    GameEntityId target,
    uint32_t effect_id,
    size_t *out_slot
) {
    if (!table || !table->slots) {
        return (size_t)-1;
    }

    for (size_t i = 0u; i < table->capacity; ++i) {
        const GameStatusEffectInstance *instance = &table->slots[i];
        if (!instance->in_use) {
            continue;
        }
        if (instance->target.index == target.index && instance->target.generation == target.generation
            && instance->effect_id == effect_id) {
            if (out_slot) {
                *out_slot = i;
            }
            return i;
        }
    }
    return (size_t)-1;
}

static GameStatusEffectResult game_status_effect_emit(
    GameEventType type,
    const GameStatusEffectInstance *instance,
    uint64_t tick,
    uint64_t parent_event_sequence,
    GameEventQueue *event_queue,
    GameEventLog *event_log
) {
    if (event_queue) {
        GameStatusEffectPayload payload = {
            .effect_id = instance->effect_id,
            .magnitude = instance->magnitude,
            .duration_ticks = instance->duration_ticks,
            .target_q = instance->target_q,
            .target_r = instance->target_r,
        };

        GameEvent event = {
            .type = type,
            .tick = tick,
            .source = instance->source,
            .payload_size = sizeof(GameStatusEffectPayload),
        };
        memcpy(event.payload.bytes, &payload, sizeof(payload));
        if (game_event_queue_push(event_queue, &event) != GAME_EVENT_QUEUE_RESULT_OK) {
            return GAME_STATUS_EFFECT_RESULT_BUFFER_TOO_SMALL;
        }
    }

    if (event_log) {
        if (game_event_log_append(
                event_log,
                tick,
                type,
                instance->source,
                parent_event_sequence,
                NULL
            )
            != GAME_EVENT_LOG_RESULT_OK) {
            return GAME_STATUS_EFFECT_RESULT_BUFFER_TOO_SMALL;
        }
    }

    return GAME_STATUS_EFFECT_RESULT_OK;
}

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
) {
    if (!game_status_effect_table_is_valid(table) || !out_handle || !game_entity_id_is_valid(target)
        || !game_entity_id_is_valid(source) || duration_ticks == 0u) {
        return GAME_STATUS_EFFECT_RESULT_INVALID_ARGUMENT;
    }

    if (registry && !game_entity_registry_is_alive(registry, target)) {
        return GAME_STATUS_EFFECT_RESULT_INVALID_HANDLE;
    }

    if (magnitude == 0) {
        return GAME_STATUS_EFFECT_RESULT_INVALID_ARGUMENT;
    }

    size_t existing = 0u;
    bool found = game_status_effect_find_match(table, target, effect_id, &existing) != (size_t)-1;

    if (found) {
        GameStatusEffectInstance *instance = &table->slots[existing];
        if (instance->duration_ticks < duration_ticks) {
            instance->duration_ticks = duration_ticks;
        }
        if (instance->magnitude < magnitude) {
            instance->magnitude = magnitude;
        }
        instance->applied_tick = current_tick;

        *out_handle = instance->handle;
        return game_status_effect_emit(
            GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED,
            instance,
            current_tick,
            parent_event_sequence,
            event_queue,
            event_log
        );
    }

    if (table->count >= table->capacity) {
        return GAME_STATUS_EFFECT_RESULT_BUFFER_TOO_SMALL;
    }

    size_t next_slot = table->capacity;
    for (size_t i = 0u; i < table->capacity; ++i) {
        if (!table->slots[i].in_use) {
            next_slot = i;
            break;
        }
    }
    if (next_slot == table->capacity) {
        return GAME_STATUS_EFFECT_RESULT_BUFFER_TOO_SMALL;
    }

    GameStatusEffectInstance *instance = &table->slots[next_slot];
    *instance = (GameStatusEffectInstance){
        .handle = {.slot = (uint32_t)next_slot, .generation = instance->generation},
        .generation = instance->generation,
        .in_use = true,
        .target = target,
        .source = source,
        .effect_id = effect_id,
        .magnitude = magnitude,
        .duration_ticks = duration_ticks,
        .applied_tick = current_tick,
        .target_q = target_q,
        .target_r = target_r,
    };

    *out_handle = instance->handle;
    ++table->count;

    return game_status_effect_emit(
        GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED,
        instance,
        current_tick,
        parent_event_sequence,
        event_queue,
        event_log
    );
}

GameStatusEffectResult game_status_effect_advance(
    GameStatusEffectTable *table,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log
) {
    if (!game_status_effect_table_is_valid(table)) {
        return GAME_STATUS_EFFECT_RESULT_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < table->capacity; ++i) {
        GameStatusEffectInstance *instance = &table->slots[i];
        if (!instance->in_use) {
            continue;
        }

        uint64_t due_tick = instance->applied_tick + instance->duration_ticks;
        if (current_tick >= due_tick) {
            GameStatusEffectResult emitted = game_status_effect_emit(
                GAME_EVENT_TYPE_STATUS_EFFECT_EXPIRED,
                instance,
                current_tick,
                GAME_EVENT_LOG_INVALID_PARENT_ID,
                event_queue,
                event_log
            );
            if (emitted != GAME_STATUS_EFFECT_RESULT_OK) {
                return emitted;
            }

            instance->in_use = false;
            ++instance->generation;
            ++instance->handle.generation;
            --table->count;
        }
    }

    return GAME_STATUS_EFFECT_RESULT_OK;
}

size_t game_status_effect_count(const GameStatusEffectTable *table) {
    return table ? table->count : 0u;
}

bool game_status_effect_handle_is_valid(const GameStatusEffectTable *table, GameStatusEffectHandle handle) {
    if (!game_status_effect_table_is_valid(table) || handle.slot >= table->capacity) {
        return false;
    }
    return game_status_effect_handle_matches(&table->slots[handle.slot], handle);
}

GameStatusEffectResult game_status_effect_get(
    const GameStatusEffectTable *table,
    GameStatusEffectHandle handle,
    GameStatusEffectInstance *out_instance
) {
    if (!game_status_effect_table_is_valid(table) || !out_instance) {
        return GAME_STATUS_EFFECT_RESULT_INVALID_ARGUMENT;
    }
    if (!game_status_effect_handle_is_valid(table, handle)) {
        return GAME_STATUS_EFFECT_RESULT_STALE_HANDLE;
    }
    *out_instance = table->slots[handle.slot];
    return GAME_STATUS_EFFECT_RESULT_OK;
}
