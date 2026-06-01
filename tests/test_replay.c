#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sim/command.h"
#include "sim/replay.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[replay] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int test_replay_deterministic_trace(void) {
    int failed = 0;
    GameTileFieldConfig field_config = {
        .q_min = -2,
        .r_min = -2,
        .q_count = 5u,
        .r_count = 5u,
        .min_value = 0,
        .max_value = 64,
    };

    GameCommandNoisePayload noise1 = {
        .origin_q = 0,
        .origin_r = 0,
        .intensity = 6,
        .max_radius = 1,
        .attenuation = 1,
        .decay_ticks = 0,
    };
    GameCommandNoisePayload noise2 = {
        .origin_q = 1,
        .origin_r = 1,
        .intensity = 8,
        .max_radius = 1,
        .attenuation = 1,
        .decay_ticks = 0,
    };

    GameCommand commands[2] = {0};
    commands[0].type = GAME_COMMAND_TYPE_EMIT_NOISE;
    commands[0].requested_tick = 0u;
    commands[0].payload_size = sizeof(noise1);
    memcpy(commands[0].payload.bytes, &noise1, sizeof(noise1));

    commands[1].type = GAME_COMMAND_TYPE_EMIT_NOISE;
    commands[1].requested_tick = 1u;
    commands[1].payload_size = sizeof(noise2);
    memcpy(commands[1].payload.bytes, &noise2, sizeof(noise2));

    GameReplayInput input = {
        .seed = 42u,
        .ticks_to_run = 2u,
        .commands = commands,
        .command_count = 2u,
        .field_config = &field_config,
    };

    char trace[1024] = {0};
    size_t trace_size = 0u;
    size_t executed = 0u;

    failed += assert_true(
        game_replay_capture_trace(&input, trace, sizeof(trace), &trace_size, &executed)
            == GAME_REPLAY_RESULT_OK,
        "capture trace for noise replay"
    );
    failed += assert_true(executed == 2u, "replay executed two commands");
    failed += assert_true(trace_size > 0u, "trace emitted");

    const char *expected_trace =
        "[event_log count=4]\n"
        "seq=1,tick=0,type=3,source=0:0,parent=18446744073709551615\n"
        "seq=2,tick=0,type=4,source=0:0,parent=1\n"
        "seq=3,tick=1,type=3,source=0:0,parent=18446744073709551615\n"
        "seq=4,tick=1,type=4,source=0:0,parent=3\n";

    char replay_again[1024] = {0};
    size_t trace_again_size = 0u;
    size_t executed_again = 0u;
    failed += assert_true(
        game_replay_capture_trace(&input, replay_again, sizeof(replay_again), &trace_again_size, &executed_again)
            == GAME_REPLAY_RESULT_OK,
        "capture second deterministic replay run"
    );
    failed += assert_true(trace_again_size == trace_size, "trace sizes stable");

    size_t diff = game_replay_trace_first_diff_line(expected_trace, trace);
    failed += assert_true(diff == 0u, "captured trace matches expected");
    diff = game_replay_trace_first_diff_line(trace, replay_again);
    failed += assert_true(diff == 0u, "replay runs compare equal");

    const char *wrong_trace =
        "[event_log count=5]\n"
        "seq=1,tick=0,type=3,source=0:0,parent=18446744073709551615\n"
        "seq=2,tick=0,type=4,source=0:0,parent=1\n"
        "seq=3,tick=1,type=3,source=0:0,parent=18446744073709551615\n"
        "seq=4,tick=1,type=4,source=0:0,parent=3\n";

    failed += assert_true(game_replay_trace_first_diff_line(wrong_trace, trace) == 1u, "trace mismatch reports first line");

    return failed;
}

int test_replay(void) {
    int failed = 0;
    failed += test_replay_deterministic_trace();

    if (failed == 0) {
        printf("[replay] PASS\n");
    }
    return failed;
}
