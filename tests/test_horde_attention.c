#include <stdio.h>

#include "sim/horde_attention.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[horde] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_horde_attention(void) {
    int failed = 0;

    GameHordeAttentionConfig config = {
        .pressurize_per_sample = 1u,
        .decay_per_tick = 1u,
        .attack_threshold = 10u,
        .calm_threshold = 6u,
        .max_pressure = 255u,
    };

    GameHordeAttentionState state = {0};
    game_horde_attention_init(&state, &config);

    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 8u) == GAME_EVENT_LOG_RESULT_OK, "horde log init");

    GameHordeAttentionUpdateResult update = {0};

    failed += assert_true(
        game_horde_attention_tick(&state, 10u, (GameEntityId){1u, 0u}, 1u, 0u, &log, &update) == GAME_HORDE_ATTENTION_RESULT_OK,
        "threat sample tick1"
    );
    failed += assert_true(update.current_posture == GAME_HORDE_POSTURE_THREATENED, "threatened posture after high sample");
    failed += assert_true(update.previous_pressure == 0u, "pressure started at zero");
    failed += assert_true(update.current_pressure == 10u, "pressure updated from sample");

    failed += assert_true(
        game_horde_attention_tick(&state, 0u, (GameEntityId){1u, 0u}, 2u, 0u, &log, &update) == GAME_HORDE_ATTENTION_RESULT_OK,
        "tick decay to threat maintained"
    );
    failed += assert_true(update.current_pressure == 9u, "pressure decays but stays elevated");
    failed += assert_true(update.current_posture == GAME_HORDE_POSTURE_THREATENED, "posture no flap");

    failed += assert_true(
        game_horde_attention_tick(&state, 0u, (GameEntityId){1u, 0u}, 3u, 0u, &log, &update) == GAME_HORDE_ATTENTION_RESULT_OK,
        "multiple decay tick1"
    );
    failed += assert_true(update.current_pressure == 8u, "pressure 8");

    failed += assert_true(
        game_horde_attention_tick(&state, 0u, (GameEntityId){1u, 0u}, 4u, 0u, &log, &update) == GAME_HORDE_ATTENTION_RESULT_OK,
        "multiple decay tick2"
    );
    failed += assert_true(update.current_pressure == 7u, "pressure 7");

    failed += assert_true(
        game_horde_attention_tick(&state, 0u, (GameEntityId){1u, 0u}, 5u, 0u, &log, &update) == GAME_HORDE_ATTENTION_RESULT_OK,
        "multiple decay tick3"
    );
    failed += assert_true(update.current_posture == GAME_HORDE_POSTURE_DORMANT, "back to dormant below calm");

    failed += assert_true(game_event_log_count(&log) > 0u, "event log captured transitions");

    game_event_log_destroy(&log);

    if (failed == 0) {
        printf("[horde] PASS\n");
    }

    return failed;
}
