#include <stdio.h>

#include "ecs/entity.h"
#include "sim/scheduler.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[scheduler] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int assert_equal_u64(unsigned long long lhs, unsigned long long rhs, const char *label) {
    if (lhs != rhs) {
        printf("[scheduler] FAIL: %s (%llu != %llu)\n", label, lhs, rhs);
        return 1;
    }
    return 0;
}

int test_scheduler(void) {
    int failed = 0;

    GameScheduler scheduler = {0};
    failed += assert_true(game_scheduler_init(&scheduler, 2u) == GAME_SCHEDULER_RESULT_OK, "scheduler init");

    GameSchedulerHandle first = {0};
    GameSchedulerHandle second = {0};
    GameSchedulerHandle third = {0};
    GameSchedulerHandle fourth = {0};

    uint8_t payload_a[1] = {1u};
    uint8_t payload_b[1] = {2u};
    uint8_t payload_c[1] = {3u};
    uint8_t payload_d[1] = {4u};

    GameEntityId source = {1u, 1u};

    failed += assert_true(
        game_scheduler_schedule(&scheduler, 10u, 1u, source, payload_a, sizeof(payload_a), &first) == GAME_SCHEDULER_RESULT_OK,
        "schedule first"
    );
    failed += assert_true(
        game_scheduler_schedule(&scheduler, 8u, 1u, source, payload_b, sizeof(payload_b), &second) == GAME_SCHEDULER_RESULT_OK,
        "schedule second"
    );
    failed += assert_true(game_scheduler_active_count(&scheduler) == 2u, "active count after schedule");

    GameScheduledEvent out_events[2] = {0};
    size_t dispatched = 0u;
    failed += assert_true(
        game_scheduler_dispatch(&scheduler, 9u, out_events, 2u, &dispatched) == GAME_SCHEDULER_RESULT_OK,
        "dispatch before earliest due"
    );
    failed += assert_true(dispatched == 1u, "dispatch due and overdue events");
    failed += assert_true(out_events[0].due_tick == 8u, "dispatch first due event");

    failed += assert_true(game_scheduler_cancel(&scheduler, first) == GAME_SCHEDULER_RESULT_OK, "cancel first");
    failed += assert_true(game_scheduler_active_count(&scheduler) == 0u, "active count after cancel");

    failed += assert_true(game_scheduler_dispatch(&scheduler, 8u, out_events, 2u, &dispatched) == GAME_SCHEDULER_RESULT_OK, "dispatch after due event");
    failed += assert_equal_u64(dispatched, 0u, "dispatched due event only once");

    failed += assert_true(
        game_scheduler_cancel(&scheduler, first) == GAME_SCHEDULER_RESULT_INVALID_HANDLE,
        "canceled handle rejected"
    );

    failed += assert_true(
        game_scheduler_schedule(&scheduler, 12u, 1u, source, payload_c, sizeof(payload_c), &third) == GAME_SCHEDULER_RESULT_OK,
        "schedule after cancel"
    );
    failed += assert_true(
        game_scheduler_schedule(&scheduler, 13u, 1u, source, payload_d, sizeof(payload_d), &fourth) == GAME_SCHEDULER_RESULT_OK,
        "schedule second after cancel and dispatch"
    );

    failed += assert_true(game_scheduler_active_count(&scheduler) == 2u, "active count includes new event");

    GameScheduledEvent ordered[2] = {0};
    size_t ordered_count = 0u;
    failed += assert_true(
        game_scheduler_dispatch(&scheduler, 13u, ordered, 2u, &ordered_count) == GAME_SCHEDULER_RESULT_OK,
        "dispatch remaining events"
    );
    failed += assert_equal_u64(ordered_count, 2u, "dispatch remaining count");

    failed += assert_true(ordered[0].due_tick <= ordered[1].due_tick, "ordered by due tick");
    failed += assert_true(
        ordered[0].payload[0] == 3u && ordered[1].payload[0] == 4u,
        "reuse order and payload deterministic"
    );

    failed += assert_true(game_scheduler_dispatch(&scheduler, 20u, ordered, 0u, &ordered_count) == GAME_SCHEDULER_RESULT_OK, "dispatch with empty out buffer zero count");
    failed += assert_true(ordered_count == 0u, "dispatch zero capacity yields zero outputs");

    game_scheduler_destroy(&scheduler);

    if (failed == 0) {
        printf("[scheduler] PASS\n");
    }

    return failed;
}
