#include "sim/scheduler.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void game_scheduler_clear_entry(GameSchedulerEntry *entry) {
    if (!entry) {
        return;
    }

    uint32_t generation = entry->generation;
    *entry = (GameSchedulerEntry){0};
    entry->generation = generation;
}

static bool game_scheduler_handle_is_valid(const GameScheduler *scheduler, GameSchedulerHandle handle) {
    if (!scheduler || !scheduler->entries) {
        return false;
    }
    if (handle.slot >= scheduler->capacity) {
        return false;
    }

    const GameSchedulerEntry *entry = &scheduler->entries[handle.slot];
    return entry->occupied && entry->generation == handle.generation;
}

GameSchedulerResult game_scheduler_init(GameScheduler *scheduler, size_t capacity) {
    if (!scheduler || capacity == 0u) {
        return GAME_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    scheduler->entries = (GameSchedulerEntry *)calloc(capacity, sizeof(GameSchedulerEntry));
    if (!scheduler->entries) {
        return GAME_SCHEDULER_RESULT_FULL;
    }

    for (size_t i = 0u; i < capacity; ++i) {
        scheduler->entries[i] = (GameSchedulerEntry){.generation = 1u};
    }

    scheduler->capacity = capacity;
    scheduler->active_count = 0u;
    scheduler->next_sequence = 1u;
    return GAME_SCHEDULER_RESULT_OK;
}

void game_scheduler_destroy_contents(GameScheduler *scheduler) {
    if (!scheduler) {
        return;
    }

    free(scheduler->entries);
    scheduler->entries = NULL;
    scheduler->capacity = 0u;
    scheduler->active_count = 0u;
    scheduler->next_sequence = 0u;
}

void game_scheduler_destroy(GameScheduler *scheduler) {
    game_scheduler_destroy_contents(scheduler);
}

static bool game_scheduler_is_payload_too_large(size_t payload_size) {
    return payload_size > GAME_SCHEDULER_MAX_PAYLOAD_BYTES;
}

GameSchedulerResult game_scheduler_schedule(
    GameScheduler *scheduler,
    uint64_t due_tick,
    uint8_t type,
    GameEntityId source,
    const void *payload,
    uint8_t payload_size,
    GameSchedulerHandle *out_handle
) {
    if (!scheduler || !scheduler->entries || !out_handle || game_scheduler_is_payload_too_large(payload_size)) {
        return GAME_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    if (payload_size > 0u && !payload) {
        return GAME_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    if (!game_entity_id_is_valid(source)) {
        return GAME_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    if (scheduler->active_count >= scheduler->capacity) {
        return GAME_SCHEDULER_RESULT_FULL;
    }

    size_t free_index = scheduler->capacity;
    for (size_t index = 0u; index < scheduler->capacity; ++index) {
        if (!scheduler->entries[index].occupied) {
            free_index = index;
            break;
        }
    }

    if (free_index >= scheduler->capacity) {
        return GAME_SCHEDULER_RESULT_FULL;
    }

    GameSchedulerEntry *entry = &scheduler->entries[free_index];
    *entry = (GameSchedulerEntry){
        .occupied = true,
        .cancelled = false,
        .fired = false,
        .generation = entry->generation,
        .due_tick = due_tick,
        .sequence = scheduler->next_sequence++,
        .type = type,
        .payload_size = payload_size,
        .reserved = 0u,
        .source = source,
    };

    if (payload_size > 0u) {
        memcpy(entry->payload, payload, payload_size);
    }

    *out_handle = (GameSchedulerHandle){.slot = (uint32_t)free_index, .generation = entry->generation};
    ++scheduler->active_count;
    return GAME_SCHEDULER_RESULT_OK;
}

GameSchedulerResult game_scheduler_cancel(GameScheduler *scheduler, GameSchedulerHandle handle) {
    if (!scheduler || !scheduler->entries) {
        return GAME_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    if (!game_scheduler_handle_is_valid(scheduler, handle)) {
        return GAME_SCHEDULER_RESULT_INVALID_HANDLE;
    }

    GameSchedulerEntry *entry = &scheduler->entries[handle.slot];
    if (entry->cancelled) {
        return GAME_SCHEDULER_RESULT_ALREADY_CANCELLED;
    }

    entry->cancelled = true;
    entry->occupied = false;
    ++entry->generation;
    --scheduler->active_count;

    game_scheduler_clear_entry(entry);
    return GAME_SCHEDULER_RESULT_OK;
}

static size_t game_scheduler_find_next_entry(const GameScheduler *scheduler, uint64_t current_tick) {
    size_t selected = scheduler->capacity;

    for (size_t i = 0u; i < scheduler->capacity; ++i) {
        const GameSchedulerEntry *entry = &scheduler->entries[i];
        if (!entry->occupied || entry->cancelled || entry->fired || entry->due_tick > current_tick) {
            continue;
        }

        if (selected >= scheduler->capacity) {
            selected = i;
            continue;
        }

        const GameSchedulerEntry *best = &scheduler->entries[selected];
        if (entry->due_tick < best->due_tick) {
            selected = i;
        } else if (entry->due_tick == best->due_tick && entry->sequence < best->sequence) {
            selected = i;
        }
    }

    return selected;
}

GameSchedulerResult game_scheduler_dispatch(
    GameScheduler *scheduler,
    uint64_t current_tick,
    GameScheduledEvent *out_events,
    size_t out_events_capacity,
    size_t *out_dispatched_count
) {
    if (!scheduler || !scheduler->entries || !out_dispatched_count) {
        return GAME_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    if (out_events == NULL && out_events_capacity != 0u) {
        return GAME_SCHEDULER_RESULT_INVALID_ARGUMENT;
    }

    *out_dispatched_count = 0u;
    if (scheduler->active_count == 0u) {
        return GAME_SCHEDULER_RESULT_OK;
    }

    bool output_too_small = false;
    while (true) {
        size_t next_index = game_scheduler_find_next_entry(scheduler, current_tick);
        if (next_index >= scheduler->capacity) {
            break;
        }

        if (*out_dispatched_count >= out_events_capacity) {
            output_too_small = true;
            break;
        }

        GameSchedulerEntry *entry = &scheduler->entries[next_index];

        if (out_events != NULL) {
            out_events[*out_dispatched_count] = (GameScheduledEvent){
                .handle = (GameSchedulerHandle){.slot = (uint32_t)next_index, .generation = entry->generation},
                .due_tick = entry->due_tick,
                .sequence = entry->sequence,
                .type = entry->type,
                .source = entry->source,
                .payload_size = entry->payload_size,
            };
            if (entry->payload_size > 0u) {
                memcpy(out_events[*out_dispatched_count].payload, entry->payload, entry->payload_size);
            }
        }

        ++*out_dispatched_count;

        entry->fired = true;
        entry->occupied = false;
        ++entry->generation;
        --scheduler->active_count;
        game_scheduler_clear_entry(entry);
    }

    if (output_too_small) {
        return GAME_SCHEDULER_RESULT_OUTPUT_TOO_SMALL;
    }

    return GAME_SCHEDULER_RESULT_OK;
}

size_t game_scheduler_active_count(const GameScheduler *scheduler) {
    return scheduler ? scheduler->active_count : 0u;
}

size_t game_scheduler_serialize(const GameScheduler *scheduler, GameSchedulerEntry *out_entries, size_t out_capacity) {
    if (!scheduler || !scheduler->entries || !out_entries) {
        return 0u;
    }

    size_t emitted = 0u;
    for (size_t i = 0u; i < scheduler->capacity; ++i) {
        const GameSchedulerEntry *entry = &scheduler->entries[i];
        if (!entry->occupied || entry->fired || entry->cancelled) {
            continue;
        }

        if (emitted >= out_capacity) {
            return emitted;
        }
        out_entries[emitted++] = *entry;
    }

    return emitted;
}
