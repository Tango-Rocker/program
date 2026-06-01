#include "event/event.h"

#include <stdio.h>

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[event] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int assert_equal_u8(unsigned int lhs, unsigned int rhs, const char *label) {
    if (lhs != rhs) {
        printf("[event] FAIL: %s (%u != %u)\n", label, lhs, rhs);
        return 1;
    }
    return 0;
}

int test_event_queue(void) {
    int failed = 0;
    GameEventQueue queue = {0};
    GameEventQueueResult result = game_event_queue_init(&queue, 2);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "event queue init");

    GameEvent e1 = {
        .type = GAME_EVENT_TYPE_TEST,
        .tick = 12u,
        .source = {1u, 1u},
        .payload_size = 4u,
    };
    e1.payload.words[0] = 1234u;
    GameEvent e2 = {
        .type = GAME_EVENT_TYPE_SIMULATION,
        .tick = 12u,
        .source = {2u, 7u},
        .payload_size = 0u,
    };

    result = game_event_queue_push(&queue, &e1);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "push first event");
    result = game_event_queue_push(&queue, &e2);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "push second event");

    failed += assert_true(game_event_queue_count(&queue) == 2u, "queue count 2");

    GameEvent first = {0};
    GameEvent second = {0};
    result = game_event_queue_pop(&queue, &first);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "pop first");
    failed += assert_equal_u8(first.type, GAME_EVENT_TYPE_TEST, "fifo first is first inserted");

    result = game_event_queue_pop(&queue, &second);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "pop second");
    failed += assert_equal_u8(second.type, GAME_EVENT_TYPE_SIMULATION, "fifo second is second inserted");

    result = game_event_queue_pop(&queue, &first);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_EMPTY, "empty pop fails");

    GameEvent e3 = {.type = GAME_EVENT_TYPE_DEBUG, .tick = 42u, .source = {3u, 3u}, .payload_size = 0u};
    result = game_event_queue_push(&queue, &e3);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "push after clear");

    result = game_event_queue_clear(&queue);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "clear queue");
    failed += assert_true(game_event_queue_count(&queue) == 0u, "queue count zero after clear");
    result = game_event_queue_pop(&queue, &first);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_EMPTY, "empty after clear");

    GameEvent huge = {.type = GAME_EVENT_TYPE_TEST, .tick = 0u, .source = {0u, 0u}, .payload_size = GAME_EVENT_MAX_PAYLOAD_BYTES + 1u};
    result = game_event_queue_push(&queue, &huge);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_BAD_PAYLOAD, "payload limit enforced");

    GameEvent e4 = {.type = GAME_EVENT_TYPE_TEST, .tick = 0u, .source = {0u, 0u}, .payload_size = 0u};
    result = game_event_queue_push(&queue, &e4);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "push for overflow test");
    GameEvent e5 = {.type = GAME_EVENT_TYPE_TEST, .tick = 0u, .source = {0u, 0u}, .payload_size = 0u};
    result = game_event_queue_push(&queue, &e5);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_OK, "push second for overflow test");
    result = game_event_queue_push(&queue, &e5);
    failed += assert_true(result == GAME_EVENT_QUEUE_RESULT_FULL, "overflow closed");
    failed += assert_true(game_event_queue_count(&queue) == 2u, "overflow does not corrupt queue");

    game_event_queue_destroy(&queue);
    if (failed == 0) {
        printf("[event] PASS\n");
    }
    return failed;
}
