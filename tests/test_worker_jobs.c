#include <stdint.h>
#include <stdio.h>

#include "colony/job_board.h"
#include "colony/worker_ai.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[worker_jobs] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int test_worker_role_match_and_selection(void) {
    int failed = 0;
    GameJobBoard board = {0};
    GameJobBoardOrderSlot slots[8] = {0};
    GameWorkerSelectionAuditEntry audits[8] = {0};
    GameWorkerSelectionAuditLog audit_log = {0};
    game_worker_ai_audit_init(&audit_log, audits, 8u);

    failed += assert_true(
        game_job_board_init(&board, slots, 8u, 2u) == GAME_JOB_BOARD_RESULT_OK,
        "job board init for role match test"
    );

    GameJobOrderHandle miner_task = {0};
    GameJobOrderHandle builder_task = {0};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){2, 0},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            4u,
            6u,
            &miner_task
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create matching miner order"
    );
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){5, 0},
            0x00000002u,
            0u,
            1u,
            1u,
            &builder_task
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create non matching builder order"
    );

    GameWorkerSelectionConfig cfg = {
        .current_tick = 1u,
        .threat_stall_threshold = 2u,
        .emergency_role_mask = GAME_WORKER_ROLE_EMERGENCY,
    };
    GameWorkerState worker = {
        .id = (GameEntityId){10u, 0u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = (GameHexAxial){0, 0},
        .threat_level = 0u,
        .has_current_job = false,
        .current_job = {0u, 0u},
    };
    GameWorkerSelectionResultData result = {0};
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker, &board, &cfg, &audit_log, &result)
            == GAME_WORKER_AI_RESULT_OK,
        "select matching worker task"
    );
    failed += assert_true(result.selected_any, "selection result selected_any");
    failed += assert_true(result.selected_job_stable_id == 1u, "selected stable_id for role match");
    failed += assert_true(result.reason == GAME_WORKER_AI_REASON_SELECTED, "selected reason code");

    GameJobOrder status = {0};
    failed += assert_true(
        game_job_board_status(&board, miner_task, &status) == GAME_JOB_BOARD_RESULT_OK,
        "selected order visible in board"
    );
    failed += assert_true(
        status.state == GAME_JOB_BOARD_STATE_RESERVED && game_entity_id_is_valid(status.reserved_by),
        "selected order reserved to miner"
    );

    return failed;
}

static int test_worker_tie_break_and_distance(void) {
    int failed = 0;
    GameJobBoard board = {0};
    GameJobBoardOrderSlot slots[8] = {0};
    GameWorkerSelectionAuditEntry audits[8] = {0};
    GameWorkerSelectionAuditLog audit_log = {0};
    game_worker_ai_audit_init(&audit_log, audits, 8u);

    failed += assert_true(
        game_job_board_init(&board, slots, 8u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "job board init for tie-break test"
    );

    GameJobOrderHandle near_urgent = {0};
    GameJobOrderHandle far_urgent = {0};
    GameJobOrderHandle tie_stable = {0};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){1, 0},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            2u,
            2u,
            &near_urgent
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create near urgent order"
    );
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){3, 0},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            2u,
            2u,
            &far_urgent
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create far urgent order"
    );
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){5, 0},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            1u,
            1u,
            &tie_stable
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create urgent fallback order for stable-id tiebreak setup"
    );

    GameWorkerSelectionConfig cfg = {
        .current_tick = 1u,
        .threat_stall_threshold = 3u,
        .emergency_role_mask = GAME_WORKER_ROLE_EMERGENCY,
    };

    GameWorkerState worker = {
        .id = (GameEntityId){11u, 0u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = (GameHexAxial){0, 0},
        .threat_level = 0u,
    };

    GameWorkerSelectionResultData result = {0};
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker, &board, &cfg, &audit_log, &result)
            == GAME_WORKER_AI_RESULT_OK,
        "select deterministic candidate"
    );
    failed += assert_true(
        result.selected_job_stable_id == 1u,
        "near urgent beats far equal urgency"
    );
    failed += assert_true(
        result.selected_distance == 1u,
        "selected distance recorded as smaller"
    );
    failed += assert_true(
        result.selected_urgency == 2u,
        "selected urgency preserved"
    );

    failed += assert_true(
        game_job_board_transition(&board, near_urgent, GAME_JOB_BOARD_STATE_DONE) == GAME_JOB_BOARD_RESULT_OK,
        "mark first selection done"
    );

    GameWorkerSelectionResultData fallback = {0};
    worker.id = (GameEntityId){12u, 0u};
    worker.has_current_job = false;
    worker.current_job = (GameJobOrderHandle){0u, 0u};
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker, &board, &cfg, &audit_log, &fallback)
            == GAME_WORKER_AI_RESULT_OK,
        "select second worker candidate"
    );
    failed += assert_true(
        fallback.selected_job_stable_id == 2u,
        "near urgent removed via done transition"
    );

    return failed;
}

static int test_worker_stale_reservation_avoidance(void) {
    int failed = 0;
    GameJobBoard board = {0};
    GameJobBoardOrderSlot slots[4] = {0};
    GameWorkerSelectionAuditEntry audits[4] = {0};
    GameWorkerSelectionAuditLog audit_log = {0};
    game_worker_ai_audit_init(&audit_log, audits, 4u);

    failed += assert_true(
        game_job_board_init(&board, slots, 4u, 2u) == GAME_JOB_BOARD_RESULT_OK,
        "job board init for stale reservation test"
    );

    GameJobOrderHandle task = {0};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){0, 1},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            4u,
            8u,
            &task
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create reserved conflict candidate"
    );

    GameWorkerSelectionConfig cfg = {
        .current_tick = 1u,
        .threat_stall_threshold = 3u,
        .emergency_role_mask = GAME_WORKER_ROLE_EMERGENCY,
    };

    GameWorkerState worker_one = {
        .id = (GameEntityId){20u, 0u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = (GameHexAxial){0, 0},
        .threat_level = 0u,
    };
    GameWorkerSelectionResultData r1 = {0};
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker_one, &board, &cfg, &audit_log, &r1)
            == GAME_WORKER_AI_RESULT_OK,
        "first worker reserves conflict job"
    );

    GameWorkerState worker_two = {
        .id = (GameEntityId){21u, 0u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = (GameHexAxial){0, 0},
        .threat_level = 0u,
    };
    GameWorkerSelectionResultData r2 = {0};
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker_two, &board, &cfg, &audit_log, &r2)
            == GAME_WORKER_AI_RESULT_NO_CANDIDATE,
        "other worker avoids reserved task"
    );
    failed += assert_true(r2.reason == GAME_WORKER_AI_REASON_STALE_RESERVATION_AVOIDED, "avoidance reason recorded");

    failed += assert_true(game_job_board_advance_tick(&board, 5u) == GAME_JOB_BOARD_RESULT_OK, "advance stale reservation expiry");

    worker_two.has_current_job = false;
    GameWorkerSelectionResultData r3 = {0};
    cfg.current_tick = 5u;
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker_two, &board, &cfg, &audit_log, &r3)
            == GAME_WORKER_AI_RESULT_OK,
        "worker claims task after stale expiry"
    );
    failed += assert_true(r3.reason == GAME_WORKER_AI_REASON_SELECTED, "post-expiry reason selected");
    failed += assert_true(r3.selected_job_stable_id == 1u, "claimed expired task");

    return failed;
}

static int test_worker_threat_preference_and_audit(void) {
    int failed = 0;
    GameJobBoard board = {0};
    GameJobBoardOrderSlot slots[8] = {0};
    GameWorkerSelectionAuditEntry audits[8] = {0};
    GameWorkerSelectionAuditLog audit_log = {0};
    game_worker_ai_audit_init(&audit_log, audits, 8u);

    failed += assert_true(
        game_job_board_init(&board, slots, 8u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "job board init for threat/audit test"
    );

    GameJobOrderHandle routine = {0};
    GameJobOrderHandle emergency = {0};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){1, 1},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            1u,
            1u,
            &routine
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create routine job"
    );
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){5, 5},
            GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
            0u,
            2u,
            4u,
            &emergency
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create emergency job"
    );

    GameWorkerSelectionConfig cfg_low_threat = {
        .current_tick = 1u,
        .threat_stall_threshold = 2u,
        .emergency_role_mask = GAME_WORKER_ROLE_EMERGENCY,
    };
    GameWorkerSelectionConfig cfg_high_threat = {
        .current_tick = 2u,
        .threat_stall_threshold = 1u,
        .emergency_role_mask = GAME_WORKER_ROLE_EMERGENCY,
    };

    GameWorkerState worker = {
        .id = (GameEntityId){30u, 0u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = (GameHexAxial){0, 0},
        .threat_level = 0u,
    };

    GameWorkerSelectionResultData low_threat_result = {0};
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker, &board, &cfg_low_threat, &audit_log, &low_threat_result)
            == GAME_WORKER_AI_RESULT_OK,
        "low threat picks routine job"
    );
    failed += assert_true(low_threat_result.selected_job_stable_id == 1u, "low threat chooses routine first");
    failed += assert_true(audit_log.count == 1u, "audit log captured low threat decision");

    worker.has_current_job = false;
    worker.current_job = (GameJobOrderHandle){0u, 0u};
    worker.threat_level = 1u;
    cfg_high_threat.current_tick = 3u;

    GameWorkerSelectionResultData high_threat_result = {0};
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker, &board, &cfg_high_threat, &audit_log, &high_threat_result)
            == GAME_WORKER_AI_RESULT_OK,
        "high threat picks emergency job"
    );
    failed += assert_true(high_threat_result.selected_job_stable_id == 2u, "high threat prefers emergency");
    failed += assert_true(
        high_threat_result.reason == GAME_WORKER_AI_REASON_SELECTED,
        "high threat selection has selected reason"
    );

    worker.has_current_job = false;
    worker.current_job = (GameJobOrderHandle){0u, 0u};
    worker.threat_level = 1u;

    GameJobOrderHandle non_emergency_only = {0};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            3u,
            (GameHexAxial){2, 2},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            1u,
            1u,
            &non_emergency_only
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create post-usage non-emergency candidate"
    );
    failed += assert_true(
        game_worker_ai_select_job_for_worker(&worker, &board, &cfg_high_threat, &audit_log, &high_threat_result)
            != GAME_WORKER_AI_RESULT_OK,
        "no emergency-only available when high threat blocks routine"
    );
    failed += assert_true(
        high_threat_result.reason == GAME_WORKER_AI_REASON_THREAT_STALLED,
        "threat stall reason emitted"
    );

    failed += assert_true(
        audit_log.count >= 3u,
        "audit log has entries for all decisions"
    );

    return failed;
}

int test_worker_jobs(void) {
    int failed = 0;
    failed += test_worker_role_match_and_selection();
    failed += test_worker_tie_break_and_distance();
    failed += test_worker_stale_reservation_avoidance();
    failed += test_worker_threat_preference_and_audit();

    if (failed == 0) {
        printf("[worker_jobs] PASS\n");
    }
    return failed;
}
