#include <stdio.h>
#include <string.h>

#include "ui/causal_report.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[causal_report] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_causal_report(void) {
    int failed = 0;
    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 8u) == GAME_EVENT_LOG_RESULT_OK, "log init");
    uint64_t first = 0u;
    uint64_t second = 0u;
    uint64_t branch = 0u;
    failed += assert_true(
        game_event_log_append(&log, 1u, GAME_EVENT_TYPE_NOISE_EMITTED, (GameEntityId){1u, 1u},
                              GAME_EVENT_LOG_INVALID_PARENT_ID, &first)
            == GAME_EVENT_LOG_RESULT_OK,
        "append root"
    );
    failed += assert_true(
        game_event_log_append(&log, 1u, GAME_EVENT_TYPE_FIELD_IMPULSE_APPLIED, (GameEntityId){1u, 1u}, first, &second)
            == GAME_EVENT_LOG_RESULT_OK,
        "append child"
    );
    failed += assert_true(
        game_event_log_append(&log, 2u, GAME_EVENT_TYPE_ACTOR_ATTENTION_UPDATED, (GameEntityId){2u, 1u}, first, &branch)
            == GAME_EVENT_LOG_RESULT_OK,
        "append branch"
    );
    failed += assert_true(
        game_event_log_append(&log, 3u, GAME_EVENT_TYPE_DEBUG, (GameEntityId){3u, 1u}, 999u, NULL)
            == GAME_EVENT_LOG_RESULT_OK,
        "append missing parent"
    );

    char report[512] = {0};
    size_t size = 0u;
    failed += assert_true(
        game_causal_report_write(&log, second, report, sizeof(report), &size) == GAME_CAUSAL_REPORT_RESULT_OK,
        "write linear report"
    );
    failed += assert_true(strstr(report, "[causal_report selected=2]") != NULL, "report header");
    failed += assert_true(strstr(report, "seq=1,tick=1,type=3") < strstr(report, "seq=2,tick=1,type=4"),
                          "causal order stable");

    char branch_report[512] = {0};
    failed += assert_true(
        game_causal_report_write(&log, branch, branch_report, sizeof(branch_report), &size)
            == GAME_CAUSAL_REPORT_RESULT_OK,
        "branch selected report"
    );
    failed += assert_true(strstr(branch_report, "seq=3,tick=2,type=5") != NULL, "branch selected");

    char missing_report[512] = {0};
    failed += assert_true(
        game_causal_report_write(&log, 4u, missing_report, sizeof(missing_report), &size)
            == GAME_CAUSAL_REPORT_RESULT_OK,
        "missing parent report"
    );
    failed += assert_true(strstr(missing_report, "missing_parent=999") != NULL, "missing parent surfaced");

    game_event_log_destroy(&log);
    if (failed == 0) {
        printf("[causal_report] PASS\n");
    }
    return failed;
}
