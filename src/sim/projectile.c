#include "sim/projectile.h"

#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "event/event_queue.h"
#include "event/event_log.h"
#include "event/event.h"
#include "sim/noise_system.h"
#include "world/hex.h"

static bool game_projectile_slots_valid(const GameProjectileTable *table) {
    return table && table->slots != NULL && table->capacity > 0u;
}

static bool game_projectile_handle_matches(const GameProjectileState *slot, GameProjectileHandle handle) {
    return slot && slot->in_flight && slot->handle.slot == handle.slot && slot->handle.generation == handle.generation;
}

GameProjectileResult game_projectile_init(
    GameProjectileTable *table,
    GameProjectileState *slots,
    size_t capacity
) {
    if (!table || (!slots && capacity > 0u)) {
        return GAME_PROJECTILE_RESULT_INVALID_ARGUMENT;
    }

    table->slots = slots;
    table->capacity = capacity;
    table->count = 0u;
    table->next_id = 1u;

    for (size_t i = 0u; i < capacity; ++i) {
        slots[i].handle = (GameProjectileHandle){.slot = (uint32_t)i, .generation = 1u};
        slots[i].in_flight = false;
    }
    return GAME_PROJECTILE_RESULT_OK;
}

void game_projectile_clear(GameProjectileTable *table) {
    if (!game_projectile_slots_valid(table)) {
        return;
    }

    for (size_t i = 0u; i < table->capacity; ++i) {
        ++table->slots[i].handle.generation;
        table->slots[i].in_flight = false;
    }
    table->count = 0u;
}

static int32_t game_projectile_sign(int32_t value) {
    if (value < 0) {
        return -1;
    }
    if (value > 0) {
        return 1;
    }
    return 0;
}

static void game_projectile_step(GameProjectileState *projectile) {
    if (projectile->current_q == projectile->target_q && projectile->current_r == projectile->target_r) {
        return;
    }

    for (uint32_t step = 0u; step < projectile->speed_per_tick; ++step) {
        int32_t dq = projectile->target_q - projectile->current_q;
        int32_t dr = projectile->target_r - projectile->current_r;
        if (dq == 0 && dr == 0) {
            break;
        }

        if (dq != 0) {
            projectile->current_q += game_projectile_sign(dq);
            projectile->target_distance--;
            continue;
        }

        if (dr != 0) {
            projectile->current_r += game_projectile_sign(dr);
            projectile->target_distance--;
        }
    }
}

GameProjectileResult game_projectile_spawn(
    GameProjectileTable *table,
    const GameProjectileSpawnRequest *request,
    uint64_t current_tick,
    GameProjectileHandle *out_handle
) {
    if (!table || !request || !out_handle || !game_projectile_slots_valid(table)) {
        return GAME_PROJECTILE_RESULT_INVALID_ARGUMENT;
    }

    if (request->remaining_lifetime_ticks <= 0 || request->speed_per_tick == 0u) {
        return GAME_PROJECTILE_RESULT_INVALID_ARGUMENT;
    }

    if (!game_entity_id_is_valid(request->source) || !game_entity_id_is_valid(request->target)) {
        return GAME_PROJECTILE_RESULT_INVALID_ARGUMENT;
    }

    if (table->count >= table->capacity) {
        return GAME_PROJECTILE_RESULT_FULL;
    }

    size_t slot_index = 0u;
    while (slot_index < table->capacity && table->slots[slot_index].in_flight) {
        ++slot_index;
    }

    if (slot_index >= table->capacity) {
        return GAME_PROJECTILE_RESULT_FULL;
    }

    GameProjectileState *slot = &table->slots[slot_index];
    *slot = (GameProjectileState){
        .handle = { (uint32_t)slot_index, slot->handle.generation },
        .payload_id = request->payload_id,
        .ability_id = request->ability_id,
        .parent_event_sequence = request->parent_event_sequence,
        .source = request->source,
        .target = request->target,
        .source_q = request->source_q,
        .source_r = request->source_r,
        .target_q = request->target_q,
        .target_r = request->target_r,
        .current_q = request->source_q,
        .current_r = request->source_r,
        .target_distance = game_hex_axial_distance(
            (GameHexAxial){request->source_q, request->source_r},
            (GameHexAxial){request->target_q, request->target_r}
        ),
        .speed_per_tick = request->speed_per_tick,
        .remaining_lifetime_ticks = request->remaining_lifetime_ticks,
        .damage = request->damage,
        .in_flight = true,
    };

    slot->projectile_id = table->next_id++;
    if (slot->projectile_id == UINT32_MAX) {
        slot->projectile_id = 1u;
    }

    *out_handle = slot->handle;
    ++table->count;
    (void)current_tick;
    return GAME_PROJECTILE_RESULT_OK;
}

static GameProjectileResult game_projectile_emit_impact(
    const GameProjectileState *projectile,
    GameEventQueue *event_queue,
    GameEventLog *event_log,
    uint64_t current_tick
) {
    if (!projectile || (!event_queue && !event_log)) {
        return GAME_PROJECTILE_RESULT_INVALID_ARGUMENT;
    }

    GameProjectileImpactPayload impact = {
        .projectile_id = projectile->projectile_id,
        .source_index = projectile->source.index,
        .source_generation = projectile->source.generation,
        .target_q = projectile->target_q,
        .target_r = projectile->target_r,
        .damage = projectile->damage,
        .ability_id = projectile->ability_id,
    };
    GameEvent impact_event = {
        .type = GAME_EVENT_TYPE_PROJECTILE_IMPACT,
        .tick = current_tick,
        .source = projectile->source,
        .payload_size = sizeof(GameProjectileImpactPayload),
    };
    memcpy(impact_event.payload.bytes, &impact, sizeof(impact));

    if (event_queue && game_event_queue_push(event_queue, &impact_event) != GAME_EVENT_QUEUE_RESULT_OK) {
        return GAME_PROJECTILE_RESULT_FULL;
    }

    if (event_log && game_event_log_append(
            event_log,
            current_tick,
            GAME_EVENT_TYPE_PROJECTILE_IMPACT,
            projectile->source,
            projectile->parent_event_sequence,
            NULL
        ) != GAME_EVENT_LOG_RESULT_OK) {
        return GAME_PROJECTILE_RESULT_FULL;
    }

    GameNoiseEmittedPayload noise = {
        .origin_q = projectile->target_q,
        .origin_r = projectile->target_r,
        .intensity = projectile->damage > 0 ? projectile->damage : 1,
        .max_radius = 1u,
        .attenuation = 1u,
        .decay_ticks = 1u,
        .source_tick = current_tick,
        .source_index = projectile->source.index,
        .source_generation = projectile->source.generation,
    };

    GameEvent noise_event = {
        .type = GAME_EVENT_TYPE_NOISE_EMITTED,
        .tick = current_tick,
        .source = projectile->source,
        .payload_size = sizeof(GameNoiseEmittedPayload),
    };
    memcpy(noise_event.payload.bytes, &noise, sizeof(noise));

    if (event_queue && game_event_queue_push(event_queue, &noise_event) != GAME_EVENT_QUEUE_RESULT_OK) {
        return GAME_PROJECTILE_RESULT_FULL;
    }
    if (event_log && game_event_log_append(
            event_log,
            current_tick,
            GAME_EVENT_TYPE_NOISE_EMITTED,
            projectile->source,
            projectile->parent_event_sequence,
            NULL
        ) != GAME_EVENT_LOG_RESULT_OK) {
        return GAME_PROJECTILE_RESULT_FULL;
    }

    return GAME_PROJECTILE_RESULT_OK;
}

GameProjectileResult game_projectile_advance_all(
    GameProjectileTable *table,
    uint64_t current_tick,
    GameEventQueue *event_queue,
    GameEventLog *event_log
) {
    if (!table || !game_projectile_slots_valid(table)) {
        return GAME_PROJECTILE_RESULT_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < table->capacity; ++i) {
        GameProjectileState *projectile = &table->slots[i];
        if (!projectile->in_flight) {
            continue;
        }

        game_projectile_step(projectile);
        --projectile->remaining_lifetime_ticks;

        if (projectile->target_distance <= 0) {
            if (game_projectile_emit_impact(projectile, event_queue, event_log, current_tick) != GAME_PROJECTILE_RESULT_OK) {
                return GAME_PROJECTILE_RESULT_INVALID_ARGUMENT;
            }

            projectile->in_flight = false;
            ++projectile->handle.generation;
            --table->count;
            continue;
        }

        if (projectile->remaining_lifetime_ticks <= 0) {
            projectile->in_flight = false;
            ++projectile->handle.generation;
            --table->count;
        }
    }

    return GAME_PROJECTILE_RESULT_OK;
}

size_t game_projectile_count(const GameProjectileTable *table) {
    return table ? table->count : 0u;
}

bool game_projectile_handle_is_valid(const GameProjectileTable *table, GameProjectileHandle handle) {
    if (!game_projectile_slots_valid(table) || handle.slot >= table->capacity) {
        return false;
    }
    return game_projectile_handle_matches(&table->slots[handle.slot], handle);
}

GameProjectileResult game_projectile_get(const GameProjectileTable *table, GameProjectileHandle handle, GameProjectileState *out_state) {
    if (!game_projectile_slots_valid(table) || !out_state) {
        return GAME_PROJECTILE_RESULT_INVALID_ARGUMENT;
    }
    if (!game_projectile_handle_is_valid(table, handle)) {
        return GAME_PROJECTILE_RESULT_STALE_HANDLE;
    }
    *out_state = table->slots[handle.slot];
    return GAME_PROJECTILE_RESULT_OK;
}
