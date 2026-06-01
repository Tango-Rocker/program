#include "ui/causal_report.h"

#include <inttypes.h>
#include <stdio.h>

static const GameEventLogEntry *game_causal_report_find(const GameEventLog *log, uint64_t sequence) {
    if (!log) {
        return NULL;
    }
    for (size_t i = 0u; i < game_event_log_count(log); ++i) {
        const GameEventLogEntry *entry = game_event_log_at(log, i);
        if (entry && entry->sequence == sequence) {
            return entry;
        }
    }
    return NULL;
}

static GameCausalReportResult game_causal_report_append(
    char *out_buffer,
    size_t capacity,
    size_t *used,
    const char *format,
    const GameEventLogEntry *entry
) {
    if (*used >= capacity) {
        return GAME_CAUSAL_REPORT_RESULT_BUFFER_TOO_SMALL;
    }

    int written = snprintf(
        out_buffer + *used,
        capacity - *used,
        format,
        entry->sequence,
        entry->tick,
        (int)entry->type,
        entry->source.index,
        entry->source.generation,
        entry->parent_sequence
    );
    if (written < 0 || (size_t)written >= capacity - *used) {
        return GAME_CAUSAL_REPORT_RESULT_BUFFER_TOO_SMALL;
    }

    *used += (size_t)written;
    return GAME_CAUSAL_REPORT_RESULT_OK;
}

GameCausalReportResult game_causal_report_write(
    const GameEventLog *log,
    uint64_t selected_sequence,
    char *out_buffer,
    size_t capacity,
    size_t *out_size
) {
    if (!log || !out_buffer || capacity == 0u || !out_size) {
        return GAME_CAUSAL_REPORT_RESULT_INVALID_ARGUMENT;
    }

    const GameEventLogEntry *selected = game_causal_report_find(log, selected_sequence);
    if (!selected) {
        return GAME_CAUSAL_REPORT_RESULT_NOT_FOUND;
    }

    const GameEventLogEntry *chain[32] = {0};
    size_t chain_count = 0u;
    const GameEventLogEntry *cursor = selected;
    bool missing_parent = false;
    uint64_t missing_sequence = 0u;

    while (cursor && chain_count < 32u) {
        chain[chain_count++] = cursor;
        if (cursor->parent_sequence == GAME_EVENT_LOG_INVALID_PARENT_ID) {
            break;
        }
        missing_sequence = cursor->parent_sequence;
        cursor = game_causal_report_find(log, cursor->parent_sequence);
        if (!cursor) {
            missing_parent = true;
            break;
        }
    }

    size_t used = 0u;
    int written = snprintf(out_buffer, capacity, "[causal_report selected=%" PRIu64 "]\n", selected_sequence);
    if (written < 0 || (size_t)written >= capacity) {
        return GAME_CAUSAL_REPORT_RESULT_BUFFER_TOO_SMALL;
    }
    used = (size_t)written;

    for (size_t read = chain_count; read > 0u; --read) {
        GameCausalReportResult result = game_causal_report_append(
            out_buffer,
            capacity,
            &used,
            "seq=%" PRIu64 ",tick=%" PRIu64 ",type=%d,source=%" PRIu32 ":%" PRIu32 ",parent=%" PRIu64 "\n",
            chain[read - 1u]
        );
        if (result != GAME_CAUSAL_REPORT_RESULT_OK) {
            return result;
        }
    }

    if (missing_parent) {
        written = snprintf(out_buffer + used, capacity - used, "missing_parent=%" PRIu64 "\n", missing_sequence);
        if (written < 0 || (size_t)written >= capacity - used) {
            return GAME_CAUSAL_REPORT_RESULT_BUFFER_TOO_SMALL;
        }
        used += (size_t)written;
    }

    *out_size = used;
    return GAME_CAUSAL_REPORT_RESULT_OK;
}
