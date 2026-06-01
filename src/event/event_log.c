#include "event/event_log.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GameEventLogResult game_event_log_init(GameEventLog *log, size_t capacity) {
    if (!log || capacity == 0) {
        return GAME_EVENT_LOG_RESULT_INVALID_ARGUMENT;
    }

    log->entries = (GameEventLogEntry *)malloc(capacity * sizeof(GameEventLogEntry));
    if (!log->entries) {
        return GAME_EVENT_LOG_RESULT_INVALID_ARGUMENT;
    }

    log->capacity = capacity;
    log->count = 0u;
    log->next_sequence = 1u;
    return GAME_EVENT_LOG_RESULT_OK;
}

void game_event_log_destroy(GameEventLog *log) {
    if (!log) {
        return;
    }

    free(log->entries);
    log->entries = NULL;
    log->capacity = 0u;
    log->count = 0u;
    log->next_sequence = 0u;
}

GameEventLogResult game_event_log_append(
    GameEventLog *log,
    uint64_t tick,
    GameEventType type,
    GameEntityId source,
    uint64_t parent_sequence,
    uint64_t *out_sequence
) {
    if (!log || !log->entries) {
        return GAME_EVENT_LOG_RESULT_INVALID_ARGUMENT;
    }

    if (log->count >= log->capacity) {
        return GAME_EVENT_LOG_RESULT_FULL;
    }

    GameEventLogEntry entry;
    entry.sequence = log->next_sequence++;
    entry.tick = tick;
    entry.type = type;
    entry.source = source;
    entry.parent_sequence = parent_sequence;

    log->entries[log->count] = entry;
    log->count++;

    if (out_sequence) {
        *out_sequence = entry.sequence;
    }

    return GAME_EVENT_LOG_RESULT_OK;
}

size_t game_event_log_count(const GameEventLog *log) {
    return log ? log->count : 0u;
}

const GameEventLogEntry *game_event_log_at(const GameEventLog *log, size_t index) {
    if (!log || !log->entries || index >= log->count) {
        return NULL;
    }

    return &log->entries[index];
}

void game_event_log_clear(GameEventLog *log) {
    if (!log) {
        return;
    }

    log->count = 0u;
}

size_t game_event_log_serialize(const GameEventLog *log, char *out_buffer, size_t buffer_size) {
    if (!log || !out_buffer || buffer_size == 0) {
        return 0u;
    }

    size_t used = 0u;
    int written = snprintf(out_buffer + used, buffer_size - used, "[event_log count=%zu]\n", log->count);
    if (written < 0) {
        return used;
    }
    used = (size_t)written;

    for (size_t i = 0; i < log->count; ++i) {
        const GameEventLogEntry *entry = &log->entries[i];
        if (used >= buffer_size) {
            break;
        }

        written = snprintf(
            out_buffer + used,
            buffer_size - used,
            "seq=%" PRIu64 ",tick=%" PRIu64 ",type=%d,source=%" PRIu32 ":%" PRIu32 ",parent=%" PRIu64 "\n",
            entry->sequence,
            entry->tick,
            (int)entry->type,
            entry->source.index,
            entry->source.generation,
            entry->parent_sequence
        );

        if (written < 0) {
            return used;
        }

        used += (size_t)written;
    }

    if (used >= buffer_size) {
        return buffer_size;
    }
    out_buffer[used] = '\0';
    return used;
}
