#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sim/command.h"
#include "sim/replay.h"

typedef struct {
    uint64_t seed;
    uint64_t ticks;
    GameTileFieldConfig field_config;
    GameCommand commands[8];
    size_t command_count;
    char expected[2048];
} GoldenReplayFixture;

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[replay_golden] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int starts_with(const char *text, const char *prefix) {
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int load_fixture(const char *path, GoldenReplayFixture *fixture) {
    FILE *handle = fopen(path, "r");
    if (!handle || !fixture) {
        if (handle) {
            fclose(handle);
        }
        return 0;
    }

    *fixture = (GoldenReplayFixture){0};
    fixture->field_config = (GameTileFieldConfig){
        .q_min = -2,
        .r_min = -2,
        .q_count = 5u,
        .r_count = 5u,
        .min_value = 0,
        .max_value = 64,
    };

    char line[256];
    int in_expected = 0;
    size_t expected_used = 0u;
    while (fgets(line, (int)sizeof(line), handle) != NULL) {
        if (in_expected) {
            size_t len = strlen(line);
            if (expected_used + len < sizeof(fixture->expected)) {
                memcpy(fixture->expected + expected_used, line, len + 1u);
                expected_used += len;
            }
            continue;
        }
        if (strcmp(line, "---\n") == 0 || strcmp(line, "---\r\n") == 0) {
            in_expected = 1;
            continue;
        }
        if (starts_with(line, "seed=")) {
            fixture->seed = strtoull(line + 5, NULL, 10);
        } else if (starts_with(line, "ticks=")) {
            fixture->ticks = strtoull(line + 6, NULL, 10);
        } else if (starts_with(line, "field=")) {
            int q_min = 0;
            int r_min = 0;
            unsigned int q_count = 0u;
            unsigned int r_count = 0u;
            int min_value = 0;
            int max_value = 0;
            if (sscanf(line + 6, "%d %d %u %u %d %d", &q_min, &r_min, &q_count, &r_count, &min_value, &max_value)
                == 6) {
                fixture->field_config = (GameTileFieldConfig){
                    .q_min = q_min,
                    .r_min = r_min,
                    .q_count = q_count,
                    .r_count = r_count,
                    .min_value = min_value,
                    .max_value = max_value,
                };
            }
        } else if (starts_with(line, "command=") && fixture->command_count < 8u) {
            unsigned long long tick = 0u;
            int q = 0;
            int r = 0;
            int intensity = 0;
            unsigned int radius = 0u;
            unsigned int attenuation = 0u;
            unsigned int decay = 0u;
            if (sscanf(line + 8, "%llu %d %d %d %u %u %u", &tick, &q, &r, &intensity, &radius, &attenuation,
                       &decay)
                == 7) {
                GameCommandNoisePayload payload = {
                    .origin_q = q,
                    .origin_r = r,
                    .intensity = intensity,
                    .max_radius = (uint16_t)radius,
                    .attenuation = (uint16_t)attenuation,
                    .decay_ticks = (uint16_t)decay,
                };
                GameCommand *command = &fixture->commands[fixture->command_count++];
                command->type = GAME_COMMAND_TYPE_EMIT_NOISE;
                command->requested_tick = tick;
                command->payload_size = sizeof(payload);
                memcpy(command->payload.bytes, &payload, sizeof(payload));
            }
        }
    }
    fclose(handle);
    return fixture->ticks > 0u && fixture->expected[0] != '\0';
}

static int run_fixture(const char *path) {
    int failed = 0;
    GoldenReplayFixture fixture = {0};
    failed += assert_true(load_fixture(path, &fixture), "fixture loads");
    if (failed != 0) {
        return failed;
    }

    GameReplayInput input = {
        .seed = fixture.seed,
        .ticks_to_run = fixture.ticks,
        .commands = fixture.commands,
        .command_count = fixture.command_count,
        .field_config = &fixture.field_config,
    };
    char actual[2048] = {0};
    size_t actual_size = 0u;
    size_t executed = 0u;
    failed += assert_true(
        game_replay_capture_trace(&input, actual, sizeof(actual), &actual_size, &executed) == GAME_REPLAY_RESULT_OK,
        "golden replay captures"
    );
    size_t diff = game_replay_trace_first_diff_line(fixture.expected, actual);
    if (diff != 0u) {
        printf("[replay_golden] first differing trace line in %s: %zu\n", path, diff);
        failed += 1;
    }
    return failed;
}

int test_replay_golden(void) {
    int failed = 0;
    failed += run_fixture("tests/fixtures/replay/combat_noise.txt");
    failed += run_fixture("tests/fixtures/replay/colony_haul.txt");
    failed += run_fixture("tests/fixtures/replay/horde_pressure.txt");

    GoldenReplayFixture fixture = {0};
    failed += assert_true(load_fixture("tests/fixtures/replay/combat_noise.txt", &fixture), "mismatch fixture loads");
    const char *wrong = "[event_log count=999]\n";
    failed += assert_true(game_replay_trace_first_diff_line(wrong, fixture.expected) == 1u, "intentional mismatch line");

    if (failed == 0) {
        printf("[replay_golden] PASS\n");
    }
    return failed;
}
