#include <stdio.h>
#include <stdint.h>

#include "colony/job_board.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[job_board] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_job_board(void) {
    int failed = 0;

    GameJobBoard board = {0};
    GameJobBoardOrderSlot slots[4] = {0};

    failed += assert_true(
        game_job_board_init(&board, slots, 4u, 2u) == GAME_JOB_BOARD_RESULT_OK,
        "job board init"
    );

    GameJobOrderHandle job_one = {0};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            0u,
            (GameHexAxial){1, 2},
            0x01u,
            0x02u,
            3u,
            5u,
            &job_one
        ) == GAME_JOB_BOARD_RESULT_OK,
        "create first job"
    );

    GameJobOrder job_one_status = {0};
    failed += assert_true(
        game_job_board_status(&board, job_one, &job_one_status) == GAME_JOB_BOARD_RESULT_OK,
        "status for first job"
    );
    failed += assert_true(job_one_status.state == GAME_JOB_BOARD_STATE_OPEN, "initial state is open");
    failed += assert_true(job_one_status.target.q == 1 && job_one_status.target.r == 2, "stored target");
    failed += assert_true(job_one_status.required_role_flags == 0x01u, "stored role flags");
    failed += assert_true(job_one_status.required_resource_flags == 0x02u, "stored resource flags");
    failed += assert_true(job_one_status.duration_min_ticks == 3u, "stored duration min");
    failed += assert_true(job_one_status.duration_max_ticks == 5u, "stored duration max");
    failed += assert_true(job_one_status.reserved == false, "new job not reserved");

    GameJobOrderHandle job_two = {0};
    GameJobOrderHandle job_three = {0};
    GameJobOrderHandle job_four = {0};

    failed += assert_true(
        game_job_board_create_order(&board, 0u, (GameHexAxial){2, 2}, 0x00u, 0x00u, 1u, 1u, &job_two) == GAME_JOB_BOARD_RESULT_OK,
        "create second job"
    );
    failed += assert_true(
        game_job_board_create_order(&board, 0u, (GameHexAxial){3, 3}, 0x00u, 0x00u, 1u, 1u, &job_three) == GAME_JOB_BOARD_RESULT_OK,
        "create third job"
    );
    failed += assert_true(
        game_job_board_create_order(&board, 0u, (GameHexAxial){4, 4}, 0x00u, 0x00u, 1u, 1u, &job_four) == GAME_JOB_BOARD_RESULT_OK,
        "create fourth job"
    );

    GameJobOrderHandle job_full = {0};
    failed += assert_true(
        game_job_board_create_order(&board, 0u, (GameHexAxial){5, 5}, 0x00u, 0x00u, 1u, 1u, &job_full) == GAME_JOB_BOARD_RESULT_NO_SLOT,
        "board full when no slot available"
    );

    GameEntityId worker_a = {10u, 1u};
    GameEntityId worker_b = {11u, 1u};

    failed += assert_true(
        game_job_board_reserve(&board, job_two, 1u, worker_a) == GAME_JOB_BOARD_RESULT_OK,
        "reserve second job for worker a"
    );
    GameJobOrder job_two_status = {0};
    failed += assert_true(
        game_job_board_status(&board, job_two, &job_two_status) == GAME_JOB_BOARD_RESULT_OK,
        "read job two status"
    );
    failed += assert_true(job_two_status.state == GAME_JOB_BOARD_STATE_RESERVED, "reserved state");
    failed += assert_true(game_entity_id_is_valid(job_two_status.reserved_by), "reserved_by valid");
    failed += assert_true(
        job_two_status.reserved_by.index == worker_a.index && job_two_status.reserved_by.generation == worker_a.generation,
        "reserved_by stores worker a"
    );
    failed += assert_true(game_entity_id_is_valid(job_two_status.reserved_by), "reservation marked reserved flag");

    failed += assert_true(
        game_job_board_reserve(&board, job_two, 1u, worker_b) == GAME_JOB_BOARD_RESULT_ALREADY_RESERVED,
        "second reservation fails with duplicate conflict"
    );

    failed += assert_true(
        game_job_board_transition(&board, job_two, GAME_JOB_BOARD_STATE_IN_PROGRESS) == GAME_JOB_BOARD_RESULT_OK,
        "transition reserved job to in_progress"
    );
    failed += assert_true(
        game_job_board_status(&board, job_two, &job_two_status) == GAME_JOB_BOARD_RESULT_OK
            && job_two_status.state == GAME_JOB_BOARD_STATE_IN_PROGRESS,
        "state becomes in_progress"
    );

    failed += assert_true(
        game_job_board_transition(&board, job_two, GAME_JOB_BOARD_STATE_STALLED) == GAME_JOB_BOARD_RESULT_OK,
        "transition to stalled"
    );
    failed += assert_true(
        game_job_board_status(&board, job_two, &job_two_status) == GAME_JOB_BOARD_RESULT_OK
            && job_two_status.state == GAME_JOB_BOARD_STATE_STALLED,
        "state is stalled"
    );

    failed += assert_true(
        game_job_board_transition(&board, job_two, GAME_JOB_BOARD_STATE_IN_PROGRESS) == GAME_JOB_BOARD_RESULT_OK,
        "resume stalled job"
    );
    failed += assert_true(
        game_job_board_transition(&board, job_two, GAME_JOB_BOARD_STATE_DONE) == GAME_JOB_BOARD_RESULT_OK,
        "complete job"
    );
    failed += assert_true(
        game_job_board_status(&board, job_two, &job_two_status) == GAME_JOB_BOARD_RESULT_OK
            && job_two_status.state == GAME_JOB_BOARD_STATE_DONE,
        "state is done"
    );

    GameJobBoardOrderState previous = job_two_status.state;
    failed += assert_true(
        game_job_board_transition(&board, job_two, GAME_JOB_BOARD_STATE_IN_PROGRESS) == GAME_JOB_BOARD_RESULT_INVALID_STATE,
        "invalid transition from done rejected"
    );
    failed += assert_true(
        game_job_board_status(&board, job_two, &job_two_status) == GAME_JOB_BOARD_RESULT_OK
            && job_two_status.state == previous,
        "state unchanged after invalid transition"
    );

    GameJobOrderHandle job_five = {0};
    failed += assert_true(
        game_job_board_create_order(&board, 0u, (GameHexAxial){9, 9}, 0x04u, 0x08u, 2u, 4u, &job_five) == GAME_JOB_BOARD_RESULT_OK,
        "create timeout order"
    );
    failed += assert_true(
        game_job_board_reserve(&board, job_five, 10u, worker_b) == GAME_JOB_BOARD_RESULT_OK,
        "reserve timeout order"
    );
    failed += assert_true(
        game_job_board_advance_tick(&board, 11u) == GAME_JOB_BOARD_RESULT_OK,
        "advance tick before reservation expiry"
    );
    failed += assert_true(
        game_job_board_status(&board, job_five, &job_two_status) == GAME_JOB_BOARD_RESULT_OK
            && job_two_status.state == GAME_JOB_BOARD_STATE_RESERVED,
        "still reserved before expiry"
    );

    failed += assert_true(
        game_job_board_advance_tick(&board, 12u) == GAME_JOB_BOARD_RESULT_OK,
        "advance into reservation expiry"
    );
    failed += assert_true(
        game_job_board_status(&board, job_five, &job_two_status) == GAME_JOB_BOARD_RESULT_OK
            && job_two_status.state == GAME_JOB_BOARD_STATE_OPEN,
        "expired reservation returns to open"
    );
    failed += assert_true(!game_entity_id_is_valid(job_two_status.reserved_by), "reservation cleared after expiry");

    if (failed == 0) {
        printf("[job_board] PASS\n");
    }
    return failed;
}
