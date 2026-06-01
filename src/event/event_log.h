#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "event/event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_EVENT_LOG_INVALID_PARENT_ID UINT64_MAX

typedef struct GameEventLogEntry {
    uint64_t sequence;
    uint64_t tick;
    uint64_t parent_sequence;
    GameEventType type;
    GameEntityId source;
} GameEventLogEntry;

typedef enum {
    GAME_EVENT_LOG_RESULT_OK = 0,
    GAME_EVENT_LOG_RESULT_INVALID_ARGUMENT = 1,
    GAME_EVENT_LOG_RESULT_FULL = 2,
} GameEventLogResult;

typedef struct GameEventLog {
    size_t capacity;
    size_t count;
    uint64_t next_sequence;
    GameEventLogEntry *entries;
} GameEventLog;

GameEventLogResult game_event_log_init(GameEventLog *log, size_t capacity);
void game_event_log_destroy(GameEventLog *log);
GameEventLogResult game_event_log_append(
    GameEventLog *log,
    uint64_t tick,
    GameEventType type,
    GameEntityId source,
    uint64_t parent_sequence,
    uint64_t *out_sequence
);
size_t game_event_log_count(const GameEventLog *log);
const GameEventLogEntry *game_event_log_at(const GameEventLog *log, size_t index);
void game_event_log_clear(GameEventLog *log);
size_t game_event_log_serialize(const GameEventLog *log, char *out_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif
