#include "sim/sim_context.h"

#include <stdio.h>

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[sim] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_sim_tick(void) {
    int failed = 0;
    GameSimContext context = {0};
    GameSimContextConfig good = {
        .rng_seed = 0xA55Au,
        .initial_entity_capacity = 8,
        .initial_event_capacity = 8,
    };

    GameSimContextConfig bad = {0};
    failed += assert_true(game_sim_context_init(&context, &bad) != GAME_SIM_CONTEXT_RESULT_OK, "invalid config rejected");
    failed += assert_true(game_sim_context_init(NULL, &good) != GAME_SIM_CONTEXT_RESULT_OK, "null context rejected");
    failed += assert_true(game_sim_context_init(&context, NULL) != GAME_SIM_CONTEXT_RESULT_OK, "null config rejected");

    failed += assert_true(game_sim_context_init(&context, &good) == GAME_SIM_CONTEXT_RESULT_OK, "context init");
    failed += assert_true(game_sim_context_is_initialized(&context), "context initialized");
    failed += assert_true(game_sim_context_tick_count(&context) == 0u, "initial tick zero");
    failed += assert_true(game_sim_context_tick(&context) == GAME_SIM_CONTEXT_RESULT_OK, "first tick");
    failed += assert_true(game_sim_context_tick_count(&context) == 1u, "tick incremented");
    game_sim_context_shutdown(&context);

    GameSimContext a = {0};
    GameSimContext b = {0};
    failed += assert_true(game_sim_context_init(&a, &good) == GAME_SIM_CONTEXT_RESULT_OK, "left repeat context init");
    failed += assert_true(game_sim_context_init(&b, &good) == GAME_SIM_CONTEXT_RESULT_OK, "right repeat context init");

    for (uint64_t i = 0u; i < 4u; ++i) {
        failed += assert_true(game_sim_context_tick(&a) == GAME_SIM_CONTEXT_RESULT_OK, "repeat tick a");
        failed += assert_true(game_sim_context_tick(&b) == GAME_SIM_CONTEXT_RESULT_OK, "repeat tick b");
    }

    failed += assert_true(game_sim_context_tick_count(&a) == game_sim_context_tick_count(&b), "tick counts match");
    failed += assert_true(game_sim_context_rng_state(&a) == game_sim_context_rng_state(&b), "rng states match after same ticks");

    game_sim_context_shutdown(&a);
    game_sim_context_shutdown(&b);

    if (failed == 0) {
        printf("[sim] PASS\n");
    }

    return failed;
}
