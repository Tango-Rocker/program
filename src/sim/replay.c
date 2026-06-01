#include "sim/replay.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "sim/noise_system.h"
#include "sim/sim_context.h"

static size_t game_replay_compare_tick_lines(const char *expected, const char *actual) {
    size_t line = 1u;
    size_t expected_index = 0u;
    size_t actual_index = 0u;

    while (true) {
        char expected_char = expected[expected_index];
        char actual_char = actual[actual_index];

        if (expected_char == '\0' && actual_char == '\0') {
            return 0u;
        }
        if (expected_char == '\0' || actual_char == '\0') {
            return line;
        }
        if (expected_char != actual_char) {
            return line;
        }
        if (expected_char == '\n') {
            ++line;
        }
        ++expected_index;
        ++actual_index;
    }
}

size_t game_replay_trace_first_diff_line(const char *expected, const char *actual) {
    if (!expected || !actual) {
        return 1u;
    }
    return game_replay_compare_tick_lines(expected, actual);
}

static GameReplayResult game_replay_map_command_result(GameCommandQueueResult result) {
    if (result == GAME_COMMAND_QUEUE_RESULT_OK) {
        return GAME_REPLAY_RESULT_OK;
    }
    return GAME_REPLAY_RESULT_COMMAND_REJECTED;
}

static GameReplayResult game_replay_map_noise_result(GameNoiseResult result) {
    if (result == GAME_NOISE_RESULT_OK) {
        return GAME_REPLAY_RESULT_OK;
    }
    if (result == GAME_NOISE_RESULT_NOT_NOISE_COMMAND || result == GAME_NOISE_RESULT_BAD_PAYLOAD) {
        return GAME_REPLAY_RESULT_NOISE_FAILED;
    }
    return GAME_REPLAY_RESULT_NOISE_FAILED;
}

GameReplayResult game_replay_capture_trace(
    const GameReplayInput *input,
    char *out_trace_buffer,
    size_t out_trace_capacity,
    size_t *out_trace_size,
    size_t *out_executed_commands
) {
    if (!input || !out_trace_buffer || out_trace_capacity == 0u) {
        return GAME_REPLAY_RESULT_INVALID_ARGUMENT;
    }

    if (!out_executed_commands) {
        return GAME_REPLAY_RESULT_INVALID_ARGUMENT;
    }

    if ((input->command_count > 0u && !input->commands) || (input->ticks_to_run == 0u)) {
        return GAME_REPLAY_RESULT_INVALID_ARGUMENT;
    }

    GameTileFieldConfig field_config = {
        .q_min = -1,
        .r_min = -1,
        .q_count = 3u,
        .r_count = 3u,
        .min_value = 0,
        .max_value = 100,
    };
    if (input->field_config) {
        field_config = *input->field_config;
    }

    GameSimContext context = {0};
    GameSimContextConfig context_config = {
        .rng_seed = input->seed,
        .initial_entity_capacity = 4u,
        .initial_event_capacity = 8u,
    };
    if (game_sim_context_init(&context, &context_config) != GAME_SIM_CONTEXT_RESULT_OK) {
        return GAME_REPLAY_RESULT_INIT_FAILED;
    }

    GameCommandQueue command_queue = {0};
    GameCommandQueueConfig queue_config = {
        .capacity = input->command_count > 0u ? input->command_count : 1u,
        .accepted_tick_window = input->ticks_to_run < UINT32_MAX ? (uint32_t)input->ticks_to_run : UINT32_MAX,
    };
    if (queue_config.accepted_tick_window == 0u) {
        queue_config.accepted_tick_window = 1u;
    }

    if (game_command_queue_init(&command_queue, &queue_config) != GAME_COMMAND_QUEUE_RESULT_OK) {
        game_sim_context_shutdown(&context);
        return GAME_REPLAY_RESULT_INIT_FAILED;
    }

    GameEventLog replay_log = {0};
    if (game_event_log_init(&replay_log, input->command_count * 2u + 4u) != GAME_EVENT_LOG_RESULT_OK) {
        game_sim_context_shutdown(&context);
        game_command_queue_destroy(&command_queue);
        return GAME_REPLAY_RESULT_INIT_FAILED;
    }

    GameEventQueue replay_events = {0};
    if (game_event_queue_init(&replay_events, 8u) != GAME_EVENT_QUEUE_RESULT_OK) {
        game_sim_context_shutdown(&context);
        game_command_queue_destroy(&command_queue);
        game_event_log_destroy(&replay_log);
        return GAME_REPLAY_RESULT_INIT_FAILED;
    }

    GameTileField replay_field = {0};
    if (game_tile_field_init(&replay_field, &field_config) != GAME_TILE_FIELD_RESULT_OK) {
        game_sim_context_shutdown(&context);
        game_command_queue_destroy(&command_queue);
        game_event_log_destroy(&replay_log);
        game_event_queue_destroy(&replay_events);
        return GAME_REPLAY_RESULT_INIT_FAILED;
    }

    GameEntityId replay_source = {UINT32_MAX, UINT32_MAX};
    if (game_entity_registry_create(&context.entity_registry, &replay_source) != GAME_ENTITY_RESULT_OK) {
        game_sim_context_shutdown(&context);
        game_command_queue_destroy(&command_queue);
        game_event_log_destroy(&replay_log);
        game_event_queue_destroy(&replay_events);
        game_tile_field_destroy(&replay_field);
        return GAME_REPLAY_RESULT_INIT_FAILED;
    }

    *out_executed_commands = 0u;
    size_t command_cursor = 0u;

    for (uint64_t current_tick = 0u; current_tick < input->ticks_to_run; ++current_tick) {
        while (command_cursor < input->command_count && input->commands[command_cursor].requested_tick == current_tick) {
            GameCommand command = input->commands[command_cursor];
            if (!game_entity_id_is_valid(command.source)
                || !game_entity_registry_is_alive(&context.entity_registry, command.source)) {
                command.source = replay_source;
            }

            GameCommandQueueResult queue_result = game_command_queue_push(
                &command_queue,
                &command,
                current_tick,
                &context.entity_registry
            );
            if (game_replay_map_command_result(queue_result) != GAME_REPLAY_RESULT_OK) {
                game_sim_context_shutdown(&context);
                game_command_queue_destroy(&command_queue);
                game_event_log_destroy(&replay_log);
                game_event_queue_destroy(&replay_events);
                game_tile_field_destroy(&replay_field);
                return game_replay_map_command_result(queue_result);
            }
            command_cursor++;
        }

        GameCommand command = {0};
        while (game_command_queue_pop(&command_queue, &command) == GAME_COMMAND_QUEUE_RESULT_OK) {
            if (command.type != GAME_COMMAND_TYPE_EMIT_NOISE) {
                game_sim_context_shutdown(&context);
                game_command_queue_destroy(&command_queue);
                game_event_log_destroy(&replay_log);
                game_event_queue_destroy(&replay_events);
                game_tile_field_destroy(&replay_field);
                return GAME_REPLAY_RESULT_NOISE_FAILED;
            }

            GameNoiseAppliedResult result = {0};
            GameNoiseResult noise_result = game_noise_emit_from_command(
                &command,
                current_tick,
                &replay_events,
                &replay_log,
                &replay_field,
                &result
            );
            if (game_replay_map_noise_result(noise_result) != GAME_REPLAY_RESULT_OK) {
                game_sim_context_shutdown(&context);
                game_command_queue_destroy(&command_queue);
                game_event_log_destroy(&replay_log);
                game_event_queue_destroy(&replay_events);
                game_tile_field_destroy(&replay_field);
                return game_replay_map_noise_result(noise_result);
            }
            *out_executed_commands += 1u;
        }

        game_sim_context_tick(&context);
    }

    size_t used = game_event_log_serialize(&replay_log, out_trace_buffer, out_trace_capacity);
    if (used == 0u) {
        game_sim_context_shutdown(&context);
        game_command_queue_destroy(&command_queue);
        game_event_log_destroy(&replay_log);
        game_event_queue_destroy(&replay_events);
        game_tile_field_destroy(&replay_field);
        return GAME_REPLAY_RESULT_OUTPUT_BUFFER_TOO_SMALL;
    }

    if (used >= out_trace_capacity) {
        out_trace_buffer[out_trace_capacity - 1u] = '\0';
        *out_trace_size = out_trace_capacity - 1u;
    } else {
        *out_trace_size = used;
    }

    if (out_trace_size && *out_trace_size > 0u && out_trace_buffer[*out_trace_size - 1u] != '\0') {
        out_trace_buffer[*out_trace_size] = '\0';
        ++(*out_trace_size);
    }

    game_sim_context_shutdown(&context);
    game_command_queue_destroy(&command_queue);
    game_event_log_destroy(&replay_log);
    game_event_queue_destroy(&replay_events);
    game_tile_field_destroy(&replay_field);
    return GAME_REPLAY_RESULT_OK;
}
