#include <stdio.h>

#include "colony/emergency.h"
#include "colony/haul_job.h"
#include "colony/inventory.h"
#include "colony/job_board.h"
#include "colony/worker_ai.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[colony_emergency] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int test_emergency_interrupts_and_resumes_routine(void) {
    int failed = 0;

    GameInventoryEntry inventory_entries[8] = {0};
    GameInventoryReservation inventory_reservations[4] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inventory_entries, 8u, inventory_reservations, 4u, 10u) == GAME_INVENTORY_RESULT_OK,
        "emergency resume: init inventory"
    );

    GameInventoryOwner emergency_source_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {401u, 1u}, 0u};
    GameInventoryOwner emergency_dest_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {402u, 1u}, 0u};
    failed += assert_true(
        game_inventory_add(&inventory, emergency_source_owner, 77u, 2u, NULL) == GAME_INVENTORY_RESULT_OK,
        "emergency resume: prefill source stock"
    );

    GameJobBoard board = {0};
    GameJobBoardOrderSlot board_slots[8] = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 8u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "emergency resume: init job board"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[4] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 4u) == GAME_HAUL_JOB_RESULT_OK,
        "emergency resume: init haul jobs"
    );
    GameHaulJobCreateInfo haul_info = {
        .worker = {700u, 1u},
        .route_request = {0u, 0u},
        .source_owner = emergency_source_owner,
        .destination_owner = emergency_dest_owner,
        .source_tile = (GameHexAxial){0, 0},
        .destination_tile = (GameHexAxial){1, 1},
        .resource_id = 77u,
        .required_amount = 2u,
        .worker_capacity = 2u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
    };
    GameHaulJobHandle active_haul = {0u, 0u};
    failed += assert_true(
        game_haul_job_create(&haul_system, &inventory, &board, 1u, &haul_info, &active_haul) == GAME_HAUL_JOB_RESULT_OK,
        "emergency resume: init emergency-active haul"
    );

    GameJobOrderHandle routine_order = {0u, 0u};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            1u,
            (GameHexAxial){4, 4},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            2u,
            4u,
            &routine_order
        ) == GAME_JOB_BOARD_RESULT_OK,
        "emergency resume: create routine work order"
    );
    failed += assert_true(
        game_job_board_reserve(&board, routine_order, 1u, (GameEntityId){900u, 1u}) == GAME_JOB_BOARD_RESULT_OK,
        "emergency resume: reserve routine order for worker"
    );
    failed += assert_true(
        game_job_board_transition(&board, routine_order, GAME_JOB_BOARD_STATE_IN_PROGRESS) == GAME_JOB_BOARD_RESULT_OK,
        "emergency resume: start routine order"
    );

    GameWorkerState worker = {
        .id = (GameEntityId){900u, 1u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = {2, 2},
        .threat_level = 0u,
        .has_current_job = true,
        .current_job = routine_order,
    };

    GameEmergencyInterruption interruptions[2] = {0};
    GameEmergencySystem emergency_system = {0};
    failed += assert_true(
        game_emergency_init(&emergency_system, interruptions, 2u) == GAME_EMERGENCY_RESULT_OK,
        "emergency resume: init emergency system"
    );

    GameEmergencyAuditEntry audits[12] = {0};
    GameEmergencyAuditLog audit_log = {0};
    game_emergency_audit_init(&audit_log, audits, 12u);

    GameEmergencyResult high_threat_update = game_emergency_update(
        &emergency_system,
        &board,
        &inventory,
        &haul_system,
        &worker,
        active_haul,
        2u,
        6u,
        3u,
        true,
        (GameHexAxial){2, 2},
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
        0u,
        1u,
        1u,
        &audit_log
    );
    failed += assert_true(high_threat_update == GAME_EMERGENCY_RESULT_OK, "emergency resume: high threat triggers intervention");

    GameJobOrder routine_status = {0};
    failed += assert_true(
        game_job_board_status(&board, routine_order, &routine_status) == GAME_JOB_BOARD_RESULT_OK,
        "emergency resume: routine status still queryable"
    );
    failed += assert_true(routine_status.state == GAME_JOB_BOARD_STATE_STALLED, "emergency resume: routine order stalls");
    failed += assert_true(
        !game_entity_id_is_valid(routine_status.reserved_by),
        "emergency resume: routine reservation released on stall"
    );

    failed += assert_true(worker.has_current_job, "emergency resume: worker still has current job");
    failed += assert_true(
        worker.current_job.slot != routine_order.slot || worker.current_job.version != routine_order.version,
        "emergency resume: worker moved onto emergency job"
    );

    GameHaulJob haul_status = {0};
    failed += assert_true(
        game_haul_job_status(&haul_system, active_haul, &haul_status) == GAME_HAUL_JOB_RESULT_OK,
        "emergency resume: active haul can still be inspected"
    );
    failed += assert_true(haul_status.state == GAME_HAUL_JOB_STATE_ABORTED, "emergency resume: active haul is aborted during threat");
    failed += assert_true(
        audit_log.count >= 1u && audits[audit_log.count - 1u].reason == GAME_EMERGENCY_AUDIT_EMERGENCY_CREATED,
        "emergency resume: audit logs emergency creation"
    );

    GameEmergencyResult repeated_update = game_emergency_update(
        &emergency_system,
        &board,
        &inventory,
        &haul_system,
        &worker,
        active_haul,
        3u,
        6u,
        3u,
        true,
        (GameHexAxial){2, 2},
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
        0u,
        1u,
        1u,
        &audit_log
    );
    failed += assert_true(
        repeated_update == GAME_EMERGENCY_RESULT_ALREADY_ACTIVE,
        "emergency resume: repeated high threat reports already active"
    );

    GameEmergencyResult clear_update = game_emergency_update(
        &emergency_system,
        &board,
        &inventory,
        &haul_system,
        &worker,
        active_haul,
        4u,
        0u,
        3u,
        true,
        (GameHexAxial){2, 2},
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
        0u,
        1u,
        1u,
        &audit_log
    );
    failed += assert_true(clear_update == GAME_EMERGENCY_RESULT_OK, "emergency resume: clearing threat resumes routine");
    failed += assert_true(
        worker.current_job.slot == routine_order.slot && worker.current_job.version == routine_order.version,
        "emergency resume: worker returns to routine order"
    );

    failed += assert_true(
        game_job_board_status(&board, routine_order, &routine_status) == GAME_JOB_BOARD_RESULT_OK,
        "emergency resume: routine status query after clear"
    );
    failed += assert_true(routine_status.state == GAME_JOB_BOARD_STATE_IN_PROGRESS, "emergency resume: routine transitions back to in_progress");
    failed += assert_true(
        game_entity_id_is_valid(routine_status.reserved_by),
        "emergency resume: routine reservation reacquired on resume"
    );

    failed += assert_true(
        audit_log.count >= 3u && audits[audit_log.count - 1u].reason == GAME_EMERGENCY_AUDIT_ROUTINE_RESUMED,
        "emergency resume: final audit records routine resumed"
    );
    return failed;
}

static int test_emergency_resume_policy_is_sticky(void) {
    int failed = 0;

    GameInventoryEntry inventory_entries[4] = {0};
    GameInventoryReservation inventory_reservations[2] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inventory_entries, 4u, inventory_reservations, 2u, 10u) == GAME_INVENTORY_RESULT_OK,
        "sticky resume: init inventory"
    );

    GameInventoryOwner source_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {611u, 1u}, 0u};
    GameInventoryOwner destination_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {612u, 1u}, 0u};
    failed += assert_true(
        game_inventory_add(&inventory, source_owner, 55u, 1u, NULL) == GAME_INVENTORY_RESULT_OK,
        "sticky resume: seed stock for haul"
    );

    GameJobBoard board = {0};
    GameJobBoardOrderSlot board_slots[4] = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 4u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "sticky resume: init board"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[2] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 2u) == GAME_HAUL_JOB_RESULT_OK,
        "sticky resume: init haul jobs"
    );

    GameHaulJobCreateInfo haul_info = {
        .worker = {710u, 1u},
        .route_request = {0u, 0u},
        .source_owner = source_owner,
        .destination_owner = destination_owner,
        .source_tile = (GameHexAxial){0, 0},
        .destination_tile = (GameHexAxial){1, 1},
        .resource_id = 55u,
        .required_amount = 1u,
        .worker_capacity = 1u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
    };
    GameHaulJobHandle active_haul = {0u, 0u};
    failed += assert_true(
        game_haul_job_create(&haul_system, &inventory, &board, 100u, &haul_info, &active_haul) == GAME_HAUL_JOB_RESULT_OK,
        "sticky resume: init active haul"
    );

    GameJobOrderHandle routine_order = {0u, 0u};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            100u,
            (GameHexAxial){4, 4},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            2u,
            4u,
            &routine_order
        ) == GAME_JOB_BOARD_RESULT_OK,
        "sticky resume: create routine work order"
    );
    failed += assert_true(
        game_job_board_reserve(&board, routine_order, 100u, (GameEntityId){920u, 1u}) == GAME_JOB_BOARD_RESULT_OK,
        "sticky resume: reserve routine work order"
    );
    failed += assert_true(
        game_job_board_transition(&board, routine_order, GAME_JOB_BOARD_STATE_IN_PROGRESS) == GAME_JOB_BOARD_RESULT_OK,
        "sticky resume: enter routine work"
    );

    GameWorkerState worker = {
        .id = {920u, 1u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = {2, 2},
        .threat_level = 0u,
        .has_current_job = true,
        .current_job = routine_order,
    };

    GameEmergencyInterruption interruptions[1] = {0};
    GameEmergencySystem emergency_system = {0};
    failed += assert_true(
        game_emergency_init(&emergency_system, interruptions, 1u) == GAME_EMERGENCY_RESULT_OK,
        "sticky resume: init emergency system"
    );

    GameEmergencyAuditEntry audits[8] = {0};
    GameEmergencyAuditLog audit_log = {0};
    game_emergency_audit_init(&audit_log, audits, 8u);

    GameEmergencyResult high_threat_update = game_emergency_update(
        &emergency_system,
        &board,
        &inventory,
        &haul_system,
        &worker,
        active_haul,
        101u,
        8u,
        3u,
        false,
        (GameHexAxial){2, 2},
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
        0u,
        1u,
        1u,
        &audit_log
    );
    failed += assert_true(high_threat_update == GAME_EMERGENCY_RESULT_OK, "sticky resume: high threat with no-resume flag engages");

    GameJobOrder routine_status = {0};
    failed += assert_true(
        game_job_board_status(&board, routine_order, &routine_status) == GAME_JOB_BOARD_RESULT_OK,
        "sticky resume: routine status query after interruption"
    );
    failed += assert_true(routine_status.state == GAME_JOB_BOARD_STATE_STALLED, "sticky resume: routine is stalled");

    GameEmergencyResult clear_update = game_emergency_update(
        &emergency_system,
        &board,
        &inventory,
        &haul_system,
        &worker,
        active_haul,
        102u,
        0u,
        3u,
        true,
        (GameHexAxial){2, 2},
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
        0u,
        1u,
        1u,
        &audit_log
    );
    failed += assert_true(
        clear_update == GAME_EMERGENCY_RESULT_OK,
        "sticky resume: threat clear should honor stored no-resume policy"
    );
    failed += assert_true(
        !worker.has_current_job,
        "sticky resume: worker should not resume routine when policy was false"
    );
    failed += assert_true(
        game_job_board_status(&board, routine_order, &routine_status) == GAME_JOB_BOARD_RESULT_OK,
        "sticky resume: routine status still queryable after clear"
    );
    failed += assert_true(routine_status.state == GAME_JOB_BOARD_STATE_ABORTED, "sticky resume: routine aborted on clear");
    failed += assert_true(
        audit_log.count >= 1u && audits[audit_log.count - 1u].reason == GAME_EMERGENCY_AUDIT_ROUTINE_ABORTED,
        "sticky resume: final audit shows routine aborted"
    );

    return failed;
}

static int test_emergency_interrupt_can_abort_when_not_resuming(void) {
    int failed = 0;

    GameInventoryEntry inventory_entries[4] = {0};
    GameInventoryReservation inventory_reservations[2] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inventory_entries, 4u, inventory_reservations, 2u, 12u) == GAME_INVENTORY_RESULT_OK,
        "emergency abort: init inventory"
    );

    GameJobBoard board = {0};
    GameJobBoardOrderSlot board_slots[4] = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 4u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "emergency abort: init job board"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[2] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 2u) == GAME_HAUL_JOB_RESULT_OK,
        "emergency abort: init haul system"
    );

    GameJobOrderHandle routine_order = {0u, 0u};
    failed += assert_true(
        game_job_board_create_order(
            &board,
            10u,
            (GameHexAxial){1, 1},
            GAME_WORKER_ROLE_STANDARD,
            0u,
            3u,
            5u,
            &routine_order
        ) == GAME_JOB_BOARD_RESULT_OK,
        "emergency abort: create routine job"
    );
    failed += assert_true(
        game_job_board_reserve(&board, routine_order, 10u, (GameEntityId){901u, 1u}) == GAME_JOB_BOARD_RESULT_OK,
        "emergency abort: reserve routine job"
    );
    failed += assert_true(
        game_job_board_transition(&board, routine_order, GAME_JOB_BOARD_STATE_IN_PROGRESS) == GAME_JOB_BOARD_RESULT_OK,
        "emergency abort: start routine work"
    );

    GameWorkerState worker = {
        .id = (GameEntityId){901u, 1u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = {0, 0},
        .threat_level = 0u,
        .has_current_job = true,
        .current_job = routine_order,
    };

    GameEmergencyInterruption interruptions[2] = {0};
    GameEmergencySystem emergency_system = {0};
    failed += assert_true(
        game_emergency_init(&emergency_system, interruptions, 2u) == GAME_EMERGENCY_RESULT_OK,
        "emergency abort: init emergency system"
    );

    GameEmergencyAuditEntry audits[8] = {0};
    GameEmergencyAuditLog audit_log = {0};
    game_emergency_audit_init(&audit_log, audits, 8u);

    GameEmergencyResult high_threat_update = game_emergency_update(
        &emergency_system,
        &board,
        &inventory,
        &haul_system,
        &worker,
        (GameHaulJobHandle){0u, 0u},
        11u,
        9u,
        3u,
        false,
        (GameHexAxial){1, 1},
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
        0u,
        1u,
        1u,
        &audit_log
    );
    failed += assert_true(high_threat_update == GAME_EMERGENCY_RESULT_OK, "emergency abort: high threat creates emergency");

    GameJobOrder routine_status = {0};
    failed += assert_true(
        game_job_board_status(&board, routine_order, &routine_status) == GAME_JOB_BOARD_RESULT_OK
            && routine_status.state == GAME_JOB_BOARD_STATE_STALLED,
        "emergency abort: routine is stalled while emergency active"
    );
    failed += assert_true(
        !game_entity_id_is_valid(routine_status.reserved_by),
        "emergency abort: routine reservation released while stalled"
    );

    GameEmergencyResult clear_update = game_emergency_update(
        &emergency_system,
        &board,
        &inventory,
        &haul_system,
        &worker,
        (GameHaulJobHandle){0u, 0u},
        12u,
        1u,
        3u,
        false,
        (GameHexAxial){1, 1},
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
        0u,
        1u,
        1u,
        &audit_log
    );
    failed += assert_true(clear_update == GAME_EMERGENCY_RESULT_OK, "emergency abort: clearing threat aborts routine when resume disabled");

    failed += assert_true(
        worker.has_current_job == false && worker.current_job.slot == 0u && worker.current_job.version == 0u,
        "emergency abort: worker leaves job after non-resume cleanup"
    );

    failed += assert_true(
        game_job_board_status(&board, routine_order, &routine_status) == GAME_JOB_BOARD_RESULT_OK,
        "emergency abort: routine status query after clear"
    );
    failed += assert_true(routine_status.state == GAME_JOB_BOARD_STATE_ABORTED, "emergency abort: routine changes to aborted");

    failed += assert_true(
        audit_log.count >= 1u && audits[audit_log.count - 1u].reason == GAME_EMERGENCY_AUDIT_ROUTINE_ABORTED,
        "emergency abort: final audit logs routine aborted"
    );
    return failed;
}

static int test_emergency_idle_when_no_routine_job(void) {
    int failed = 0;

    GameJobBoard board = {0};
    GameJobBoardOrderSlot board_slots[2] = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 2u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "emergency idle: init board"
    );

    GameInventoryEntry inventory_entries[2] = {0};
    GameInventoryReservation inventory_reservations[1] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inventory_entries, 2u, inventory_reservations, 1u, 10u) == GAME_INVENTORY_RESULT_OK,
        "emergency idle: init inventory"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[1] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 1u) == GAME_HAUL_JOB_RESULT_OK,
        "emergency idle: init haul system"
    );

    GameEmergencyInterruption interrupts[1] = {0};
    GameEmergencySystem emergency_system = {0};
    failed += assert_true(
        game_emergency_init(&emergency_system, interrupts, 1u) == GAME_EMERGENCY_RESULT_OK,
        "emergency idle: init emergency system"
    );

    GameWorkerState worker = {
        .id = {905u, 1u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = {0, 0},
        .threat_level = 0u,
        .has_current_job = false,
        .current_job = {0u, 0u},
    };

    GameEmergencyAuditEntry audits[4] = {0};
    GameEmergencyAuditLog audit_log = {0};
    game_emergency_audit_init(&audit_log, audits, 4u);

    GameEmergencyResult idle_update = game_emergency_update(
        &emergency_system,
        &board,
        &inventory,
        &haul_system,
        &worker,
        (GameHaulJobHandle){0u, 0u},
        13u,
        7u,
        3u,
        true,
        (GameHexAxial){0, 0},
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY,
        0u,
        1u,
        1u,
        &audit_log
    );
    failed += assert_true(idle_update == GAME_EMERGENCY_RESULT_OK, "emergency idle: high threat with no routine job is benign");
    failed += assert_true(!worker.has_current_job, "emergency idle: worker remains without job");
    failed += assert_true(
        audit_log.count >= 1u && audits[audit_log.count - 1u].reason == GAME_EMERGENCY_AUDIT_IDLE,
        "emergency idle: audit shows idle branch"
    );

    return failed;
}

int test_colony_emergency(void) {
    int failed = 0;
    failed += test_emergency_interrupts_and_resumes_routine();
    failed += test_emergency_resume_policy_is_sticky();
    failed += test_emergency_interrupt_can_abort_when_not_resuming();
    failed += test_emergency_idle_when_no_routine_job();

    if (failed == 0) {
        printf("[colony_emergency] PASS\n");
    }
    return failed;
}
