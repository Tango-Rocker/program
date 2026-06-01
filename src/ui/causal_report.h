#pragma once

#include <stddef.h>
#include <stdint.h>

#include "event/event_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_CAUSAL_REPORT_RESULT_OK = 0,
    GAME_CAUSAL_REPORT_RESULT_INVALID_ARGUMENT = 1,
    GAME_CAUSAL_REPORT_RESULT_NOT_FOUND = 2,
    GAME_CAUSAL_REPORT_RESULT_BUFFER_TOO_SMALL = 3,
} GameCausalReportResult;

GameCausalReportResult game_causal_report_write(
    const GameEventLog *log,
    uint64_t selected_sequence,
    char *out_buffer,
    size_t capacity,
    size_t *out_size
);

#ifdef __cplusplus
}
#endif
