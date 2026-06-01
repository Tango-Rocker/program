#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sim/command.h"
#include "world/tile_field.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_REPLAY_RESULT_OK = 0,
    GAME_REPLAY_RESULT_INVALID_ARGUMENT = 1,
    GAME_REPLAY_RESULT_INIT_FAILED = 2,
    GAME_REPLAY_RESULT_COMMAND_REJECTED = 3,
    GAME_REPLAY_RESULT_NOISE_FAILED = 4,
    GAME_REPLAY_RESULT_OUTPUT_BUFFER_TOO_SMALL = 5,
} GameReplayResult;

typedef struct {
    uint64_t seed;
    uint64_t ticks_to_run;
    const GameCommand *commands;
    size_t command_count;
    const GameTileFieldConfig *field_config;
} GameReplayInput;

GameReplayResult game_replay_capture_trace(
    const GameReplayInput *input,
    char *out_trace_buffer,
    size_t out_trace_capacity,
    size_t *out_trace_size,
    size_t *out_executed_commands
);

size_t game_replay_trace_first_diff_line(const char *expected, const char *actual);

#ifdef __cplusplus
}
#endif
