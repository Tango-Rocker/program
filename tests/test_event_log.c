#include <stdio.h>
#include <string.h>

#include "event/event_log.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[event_log] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_event_log(void) {
    int failed = 0;
    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 3u) == GAME_EVENT_LOG_RESULT_OK, "log init");

    uint64_t seq0 = 0u;
    uint64_t seq1 = 0u;
    GameEntityId source = {2u, 0u};
    failed += assert_true(
        game_event_log_append(&log, 1u, GAME_EVENT_TYPE_NOISE_EMITTED, source, GAME_EVENT_LOG_INVALID_PARENT_ID, &seq0)
            == GAME_EVENT_LOG_RESULT_OK,
        "append first entry"
    );
    failed += assert_true(
        game_event_log_append(&log, 2u, GAME_EVENT_TYPE_FIELD_IMPULSE_APPLIED, source, seq0, &seq1)
            == GAME_EVENT_LOG_RESULT_OK,
        "append second entry"
    );
    failed += assert_true(game_event_log_count(&log) == 2u, "log count 2");

    const GameEventLogEntry *first = game_event_log_at(&log, 0u);
    const GameEventLogEntry *second = game_event_log_at(&log, 1u);
    failed += assert_true(first && first->sequence == seq0, "first sequence");
    failed += assert_true(second && second->parent_sequence == seq0, "parent sequence");

    failed += assert_true(seq0 == 1u, "first sequence fixed 1");
    failed += assert_true(seq1 == 2u, "second sequence fixed 2");

    char buffer[512];
    size_t used = game_event_log_serialize(&log, buffer, sizeof(buffer));
    failed += assert_true(used > 0u, "serialize non-zero");
    failed += assert_true(strstr(buffer, "seq=1,tick=1") != NULL, "serialize includes first");
    failed += assert_true(strstr(buffer, "seq=2,tick=2") != NULL, "serialize includes second");

    failed += assert_true(game_event_log_append(&log, 3u, GAME_EVENT_TYPE_THREAT_STATE_CHANGED, source, seq1, NULL) == GAME_EVENT_LOG_RESULT_OK, "append third");
    failed += assert_true(game_event_log_append(&log, 4u, GAME_EVENT_TYPE_ACTOR_ATTENTION_UPDATED, source, seq1, NULL)
                           == GAME_EVENT_LOG_RESULT_FULL,
                       "append overflow");

    game_event_log_destroy(&log);
    if (failed == 0) {
        printf("[event_log] PASS\n");
    }
    return failed;
}
