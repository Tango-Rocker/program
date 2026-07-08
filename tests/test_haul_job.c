#include <limits.h>
#include <stdio.h>
#include <stdint.h>

#include "colony/haul_job.h"
#include "colony/inventory.h"
#include "colony/job_board.h"
#include "colony/worker_ai.h"
#include "nav/path_service.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[haul_job] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int test_haul_job_successful_transfer(void) {
    int failed = 0;

    GameInventoryEntry inventory_entries[6] = {0};
    GameInventoryReservation inventory_reservations[4] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inventory_entries, 6u, inventory_reservations, 4u, 12u) == GAME_INVENTORY_RESULT_OK,
        "haul: init inventory"
    );

    GameInventoryOwner source_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {1u, 1u}, 0u};
    GameInventoryOwner destination_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {2u, 1u}, 0u};
    failed += assert_true(
        game_inventory_add(&inventory, source_owner, 7u, 6u, NULL) == GAME_INVENTORY_RESULT_OK,
        "haul: add source stock"
    );

    GameJobBoardOrderSlot board_slots[4] = {0};
    GameJobBoard board = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 4u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "haul: init job board"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[4] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 4u) == GAME_HAUL_JOB_RESULT_OK,
        "haul: init haul system"
    );

    uint16_t map_costs[3u] = {1u, 1u, 1u};
    GamePathCostMap cost_map = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 1u, .r_count = 3u},
        .cost = map_costs,
        .blocked_cost = UINT16_MAX,
    };
    uint32_t path_g[3u] = {0u, 0u, 0u};
    uint32_t path_f[3u] = {0u, 0u, 0u};
    int32_t path_parent[3u] = {-1, -1, -1};
    bool path_open[3u] = {0};
    bool path_closed[3u] = {0};
    bool path_touched_flags[3u] = {0};
    size_t path_heap[3u] = {0};
    size_t path_heap_pos[3u] = {0};
    size_t path_touched[3u] = {0};
    GamePathQueryScratch scratch = {
        .capacity = 3u,
        .g_score = path_g,
        .f_score = path_f,
        .parent = path_parent,
        .open = path_open,
        .closed = path_closed,
        .touched_flags = path_touched_flags,
        .heap = path_heap,
        .heap_pos = path_heap_pos,
        .touched = path_touched,
        .heap_capacity = 3u,
        .heap_pos_capacity = 3u,
        .touched_capacity = 3u,
    };

    GamePathServiceRequestSlot path_slots[1u] = {0};
    GamePathService path_service = {0};
    failed += assert_true(
        game_path_service_init(&path_service, path_slots, 1u, 3u) == GAME_PATH_SERVICE_RESULT_OK,
        "haul: init path service"
    );

    GameHexAxial path_buffer[8] = {0};
    GamePathRequestHandle route = {0u, 0u};
    failed += assert_true(
        game_path_service_submit(
            &path_service,
            0u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 2},
            &cost_map,
            &scratch,
            10u,
            path_buffer,
            8u,
            &route
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "haul: submit successful route"
    );
    failed += assert_true(
        game_path_service_advance_tick(&path_service, 1u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "haul: resolve successful route"
    );

    GameHaulJobAuditEntry audits[16] = {0};
    GameHaulJobAuditLog audit_log = {0};
    game_haul_job_audit_init(&audit_log, audits, 16u);

    GameHaulJobCreateInfo create_info = {
        .worker = (GameEntityId){99u, 1u},
        .route_request = route,
        .source_owner = source_owner,
        .destination_owner = destination_owner,
        .source_tile = (GameHexAxial){0, 0},
        .destination_tile = (GameHexAxial){0, 1},
        .resource_id = 7u,
        .required_amount = 3u,
        .worker_capacity = 3u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 2u,
    };

    GameHaulJobHandle job = {0u, 0u};
    failed += assert_true(
        game_haul_job_create(
            &haul_system,
            &inventory,
            &board,
            1u,
            &create_info,
            &job
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul: create successful job"
    );

    GameHaulJob state = {0};
    failed += assert_true(
        game_haul_job_status(&haul_system, job, &state) == GAME_HAUL_JOB_RESULT_OK,
        "haul: query created job"
    );
    failed += assert_true(state.state == GAME_HAUL_JOB_STATE_RESERVE_SOURCE, "haul: initial job state");

    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_RESERVE_SOURCE,
            10u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul: reserve source"
    );

    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_PICKUP,
            3u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul: pickup"
    );

    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_IN_TRANSIT,
            4u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul: in transit"
    );

    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_DELIVERY,
            5u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul: delivery"
    );

    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_DONE,
            6u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul: done"
    );

    failed += assert_true(game_haul_job_status(&haul_system, job, &state) == GAME_HAUL_JOB_RESULT_OK,
        "haul: query final state");
    failed += assert_true(state.state == GAME_HAUL_JOB_STATE_DONE, "haul: final state is done");
    failed += assert_true(state.delivered_amount == 3u, "haul: delivered amount");

    GameInventoryEntryView source_query[8] = {0};
    failed += assert_true(
        state.source_reservation.slot == 0u && state.source_reservation.version == 0u,
        "haul: source reservation handle cleared after completion"
    );

    size_t source_count = game_inventory_dump_owner(&inventory, source_owner, source_query, 8u);
    failed += assert_true(source_count == 1u && source_query[0].resource_id == 7u && source_query[0].total_quantity == 3u,
        "haul: source retains source quantity after completion");
    GameInventoryEntryView destination_query[8] = {0};
    size_t destination_count = game_inventory_dump_owner(&inventory, destination_owner, destination_query, 8u);
    failed += assert_true(destination_count == 1u && destination_query[0].resource_id == 7u && destination_query[0].total_quantity == 3u,
        "haul: destination received resources");

    failed += assert_true(audit_log.count >= 5u, "haul: audit trail captured");

    return failed;
}

static int test_haul_job_resource_conflict(void) {
    int failed = 0;

    GameInventoryEntry inventory_entries[4] = {0};
    GameInventoryReservation inventory_reservations[2] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inventory_entries, 4u, inventory_reservations, 2u, 8u) == GAME_INVENTORY_RESULT_OK,
        "haul conflict: init inventory"
    );

    GameInventoryOwner source_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {3u, 1u}, 0u};
    GameInventoryOwner destination_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {4u, 1u}, 0u};
    failed += assert_true(
        game_inventory_add(&inventory, source_owner, 9u, 1u, NULL) == GAME_INVENTORY_RESULT_OK,
        "haul conflict: source has limited inventory"
    );

    GameJobBoard board = {0};
    GameJobBoardOrderSlot board_slots[2] = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 2u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "haul conflict: init job board"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[2] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 2u) == GAME_HAUL_JOB_RESULT_OK,
        "haul conflict: init haul system"
    );

    uint16_t map_costs[3u] = {1u, 1u, 1u};
    GamePathCostMap cost_map = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 1u, .r_count = 3u},
        .cost = map_costs,
        .blocked_cost = UINT16_MAX,
    };
    uint32_t path_g[3u] = {0u, 0u, 0u};
    uint32_t path_f[3u] = {0u, 0u, 0u};
    int32_t path_parent[3u] = {-1, -1, -1};
    bool path_open[3u] = {0};
    bool path_closed[3u] = {0};
    bool path_touched_flags[3u] = {0};
    size_t path_heap[3u] = {0};
    size_t path_heap_pos[3u] = {0};
    size_t path_touched[3u] = {0};
    GamePathQueryScratch scratch = {
        .capacity = 3u,
        .g_score = path_g,
        .f_score = path_f,
        .parent = path_parent,
        .open = path_open,
        .closed = path_closed,
        .touched_flags = path_touched_flags,
        .heap = path_heap,
        .heap_pos = path_heap_pos,
        .touched = path_touched,
        .heap_capacity = 3u,
        .heap_pos_capacity = 3u,
        .touched_capacity = 3u,
    };
    GamePathServiceRequestSlot path_slots[1u] = {0};
    GamePathService path_service = {0};
    failed += assert_true(
        game_path_service_init(&path_service, path_slots, 1u, 3u) == GAME_PATH_SERVICE_RESULT_OK,
        "haul conflict: init path service"
    );

    GameHexAxial path_buffer[8] = {0};
    GamePathRequestHandle route = {0u, 0u};
    failed += assert_true(
        game_path_service_submit(
            &path_service,
            0u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 2},
            &cost_map,
            &scratch,
            10u,
            path_buffer,
            8u,
            &route
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "haul conflict: submit route"
    );
    failed += assert_true(
        game_path_service_advance_tick(&path_service, 1u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "haul conflict: resolve route"
    );

    GameHaulJobCreateInfo create_info = {
        .worker = (GameEntityId){12u, 1u},
        .route_request = route,
        .source_owner = source_owner,
        .destination_owner = destination_owner,
        .source_tile = (GameHexAxial){0, 0},
        .destination_tile = (GameHexAxial){0, 2},
        .resource_id = 9u,
        .required_amount = 3u,
        .worker_capacity = 3u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
    };

    GameHaulJobHandle job = {0u, 0u};
    failed += assert_true(
        game_haul_job_create(&haul_system, &inventory, &board, 2u, &create_info, &job)
        == GAME_HAUL_JOB_RESULT_OK,
        "haul conflict: create oversized demand job"
    );

    GameHaulJobAuditEntry audits[4] = {0};
    GameHaulJobAuditLog audit_log = {0};
    game_haul_job_audit_init(&audit_log, audits, 4u);
    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_RESERVE_SOURCE,
            3u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_INVENTORY_ERROR,
        "haul conflict: reserve fails with inventory shortage"
    );

    GameHaulJob status = {0};
    failed += assert_true(
        game_haul_job_status(&haul_system, job, &status) == GAME_HAUL_JOB_RESULT_OK,
        "haul conflict: query stalled state"
    );
    failed += assert_true(status.state == GAME_HAUL_JOB_STATE_STALLED, "haul conflict: state becomes stalled");

    failed += assert_true(
        game_haul_job_clear(&haul_system, &board, job) == GAME_HAUL_JOB_RESULT_OK,
        "haul conflict: clear failed job"
    );
    return failed;
}

static int test_haul_job_path_failure_and_abort(void) {
    int failed = 0;

    GameInventoryEntry inventory_entries[4] = {0};
    GameInventoryReservation inventory_reservations[2] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inventory_entries, 4u, inventory_reservations, 2u, 8u) == GAME_INVENTORY_RESULT_OK,
        "haul path: init inventory"
    );

    GameInventoryOwner source_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {5u, 1u}, 0u};
    GameInventoryOwner destination_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {6u, 1u}, 0u};
    failed += assert_true(
        game_inventory_add(&inventory, source_owner, 11u, 4u, NULL) == GAME_INVENTORY_RESULT_OK,
        "haul path: add source stock"
    );

    GameJobBoard board = {0};
    GameJobBoardOrderSlot board_slots[2] = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 2u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "haul path: init board"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[2] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 2u) == GAME_HAUL_JOB_RESULT_OK,
        "haul path: init haul system"
    );

    uint16_t blocked_costs[4u] = {1u, UINT16_MAX, UINT16_MAX, 1u};
    GamePathCostMap blocked_map = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 2u, .r_count = 2u},
        .cost = blocked_costs,
        .blocked_cost = UINT16_MAX,
    };
    uint32_t blocked_g[4u] = {0u, 0u, 0u, 0u};
    uint32_t blocked_f[4u] = {0u, 0u, 0u, 0u};
    int32_t blocked_parent[4u] = {-1, -1, -1, -1};
    bool blocked_open[4u] = {0};
    bool blocked_closed[4u] = {0};
    bool blocked_touched_flags[4u] = {0};
    size_t blocked_heap[4u] = {0};
    size_t blocked_heap_pos[4u] = {0};
    size_t blocked_touched[4u] = {0};
    GamePathQueryScratch blocked_scratch = {
        .capacity = 4u,
        .g_score = blocked_g,
        .f_score = blocked_f,
        .parent = blocked_parent,
        .open = blocked_open,
        .closed = blocked_closed,
        .touched_flags = blocked_touched_flags,
        .heap = blocked_heap,
        .heap_pos = blocked_heap_pos,
        .touched = blocked_touched,
        .heap_capacity = 4u,
        .heap_pos_capacity = 4u,
        .touched_capacity = 4u,
    };
    GamePathServiceRequestSlot path_slots[1u] = {0};
    GamePathService path_service = {0};
    failed += assert_true(
        game_path_service_init(&path_service, path_slots, 1u, 3u) == GAME_PATH_SERVICE_RESULT_OK,
        "haul path: init blocked path service"
    );

    GameHexAxial blocked_route_buffer[8] = {0};
    GamePathRequestHandle route = {0u, 0u};
    failed += assert_true(
        game_path_service_submit(
            &path_service,
            0u,
            (GameHexAxial){0, 0},
            (GameHexAxial){1, 1},
            &blocked_map,
            &blocked_scratch,
            2u,
            blocked_route_buffer,
            8u,
            &route
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "haul path: submit blocked route"
    );
    failed += assert_true(
        game_path_service_advance_tick(&path_service, 1u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "haul path: resolve blocked route"
    );

    GameHaulJobCreateInfo create_info = {
        .worker = (GameEntityId){15u, 1u},
        .route_request = route,
        .source_owner = source_owner,
        .destination_owner = destination_owner,
        .source_tile = (GameHexAxial){0, 0},
        .destination_tile = (GameHexAxial){1, 1},
        .resource_id = 11u,
        .required_amount = 2u,
        .worker_capacity = 2u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
    };

    GameHaulJobHandle job = {0u, 0u};
    failed += assert_true(
        game_haul_job_create(&haul_system, &inventory, &board, 3u, &create_info, &job) == GAME_HAUL_JOB_RESULT_OK,
        "haul path: create blocked path job"
    );

    GameHaulJobAuditEntry audits[8] = {0};
    GameHaulJobAuditLog audit_log = {0};
    game_haul_job_audit_init(&audit_log, audits, 8u);

    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_RESERVE_SOURCE,
            4u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul path: reserve source"
    );
    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_PICKUP,
            4u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul path: pickup before failed route"
    );
    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_IN_TRANSIT,
            5u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_PATH_FAILED,
        "haul path: transit fails when route is blocked"
    );

    GameHaulJob status = {0};
    failed += assert_true(
        game_haul_job_status(&haul_system, job, &status) == GAME_HAUL_JOB_RESULT_OK,
        "haul path: query failed job"
    );
    failed += assert_true(status.state == GAME_HAUL_JOB_STATE_STALLED, "haul path: failed route stalls job");

    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            job,
            GAME_HAUL_JOB_STATE_ABORTED,
            6u,
            NULL,
            &audit_log
        ) == GAME_HAUL_JOB_RESULT_OK,
        "haul path: abort stalled job"
    );

    GameInventoryEntryView source_query[4] = {0};
    size_t source_count = game_inventory_dump_owner(&inventory, source_owner, source_query, 4u);
    failed += assert_true(source_count == 1u && source_query[0].total_quantity == 4u,
        "haul path: abort returns all reserved stock");

    failed += assert_true(audit_log.count >= 4u, "haul path: audit recorded path failure and abort");

    return failed;
}

int test_haul_job(void) {
    int failed = 0;
    failed += test_haul_job_successful_transfer();
    failed += test_haul_job_resource_conflict();
    failed += test_haul_job_path_failure_and_abort();

    if (failed == 0) {
        printf("[haul_job] PASS\n");
    }
    return failed;
}
