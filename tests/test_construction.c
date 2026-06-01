#include <stdio.h>

#include "colony/construction.h"
#include "colony/haul_job.h"
#include "colony/inventory.h"
#include "colony/job_board.h"
#include "colony/worker_ai.h"
#include "nav/path_service.h"
#include "world/structure.h"
#include "world/world_map.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[construction] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int test_construction_completion_flow(void) {
    int failed = 0;

    GameWorldMap map = {0};
    failed += assert_true(
        game_world_map_init(&map, 4, -1) == GAME_WORLD_MAP_RESULT_OK,
        "construction: init world map"
    );
    GameHexAxial target_tile = {2, 2};
    failed += assert_true(
        game_world_map_create_chunk(&map, game_hex_cube_to_chunk_key(game_hex_axial_to_cube(target_tile), 4))
            == GAME_WORLD_MAP_RESULT_OK,
        "construction: create construction chunk"
    );

    static const GameStructureFootprintOffset wall_footprint[1] = {{0, 0}};
    static const GameStructureDefinition structure_definitions[] = {
        {101u, GAME_STRUCTURE_TILE_FLAG_BLOCKS_PATH, GAME_STRUCTURE_FIELD_FLAG_NONE, 1u, wall_footprint},
    };
    GameStructurePlacement structure_placements[4] = {0};
    GameStructureSystem structure_system = {0};
    failed += assert_true(
        game_structure_init(
            &structure_system,
            structure_definitions,
            1u,
            structure_placements,
            4u,
            1u
        ) == GAME_STRUCTURE_RESULT_OK,
        "construction: init structure system"
    );

    GameInventoryEntry inv_entries[8] = {0};
    GameInventoryReservation inv_reservations[8] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inv_entries, 8u, inv_reservations, 8u, 12u) == GAME_INVENTORY_RESULT_OK,
        "construction: init inventory"
    );

    GameInventoryOwner source_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {31u, 1u}, 0u};
    GameInventoryOwner destination_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {32u, 1u}, 0u};
    failed += assert_true(
        game_inventory_add(&inventory, source_owner, 7u, 6u, NULL) == GAME_INVENTORY_RESULT_OK,
        "construction: source stock for haul"
    );

    GameJobBoard board = {0};
    GameJobBoardOrderSlot board_slots[8] = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 8u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "construction: init job board"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[4] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 4u) == GAME_HAUL_JOB_RESULT_OK,
        "construction: init haul job system"
    );

    GameConstructionSystem worksite_system = {0};
    GameConstructionWorksite construction_slots[4] = {0};
    failed += assert_true(
        game_construction_init(&worksite_system, construction_slots, 4u, 1u) == GAME_CONSTRUCTION_RESULT_OK,
        "construction: init construction system"
    );

    uint16_t cost_map_values[2] = {1u, 1u};
    GamePathCostMap cost_map = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 1u, .r_count = 2u},
        .cost = cost_map_values,
        .blocked_cost = UINT16_MAX,
    };
    uint32_t scratch_g[2u] = {0u, 0u};
    uint32_t scratch_f[2u] = {0u, 0u};
    int32_t scratch_parent[2u] = {-1, -1};
    bool scratch_open[2u] = {0};
    bool scratch_closed[2u] = {0};
    GamePathQueryScratch scratch = {
        .capacity = 2u,
        .g_score = scratch_g,
        .f_score = scratch_f,
        .parent = scratch_parent,
        .open = scratch_open,
        .closed = scratch_closed,
    };

    GamePathService path_service = {0};
    GamePathServiceRequestSlot path_slots[1u] = {0};
    failed += assert_true(
        game_path_service_init(&path_service, path_slots, 1u, 3u) == GAME_PATH_SERVICE_RESULT_OK,
        "construction: init path service"
    );
    GameHexAxial path_buffer[8] = {0};
    GamePathRequestHandle route = {0u, 0u};
    failed += assert_true(
        game_path_service_submit(
            &path_service,
            0u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 1},
            &cost_map,
            &scratch,
            2u,
            path_buffer,
            8u,
            &route
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "construction: submit haul route"
    );
    failed += assert_true(
        game_path_service_advance_tick(&path_service, 1u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "construction: resolve haul route"
    );

    GameConstructionAuditEntry audit_entries[16] = {0};
    GameConstructionAuditLog audit_log = {0};
    game_construction_audit_init(&audit_log, audit_entries, 16u);

    GameConstructionResourceRequirement requirements[1] = {{7u, 4u, 0u}};
    GameConstructionCreateInfo create_info = {
        .target_tile = target_tile,
        .structure_definition_id = 101u,
        .assigned_worker = (GameEntityId){88u, 1u},
        .resource_source_owner = source_owner,
        .resource_destination_owner = destination_owner,
        .required_work_ticks = 2u,
        .worker_capacity = 4u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .required_resource_flags = 0u,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
        .haul_route_request = route,
        .requirements = requirements,
        .requirement_count = 1u,
    };

    GameConstructionWorksiteHandle worksite_handle = {0u, 0u};
    failed += assert_true(
        game_construction_create(
            &worksite_system,
            &board,
            &inventory,
            1u,
            &create_info,
            &worksite_handle
        ) == GAME_CONSTRUCTION_RESULT_OK,
        "construction: create construction worksite"
    );

    GameConstructionWorksite worksite = {0};
    failed += assert_true(
        game_construction_status(&worksite_system, worksite_handle, &worksite) == GAME_CONSTRUCTION_RESULT_OK,
        "construction: query created worksite"
    );
    failed += assert_true(worksite.state == GAME_CONSTRUCTION_STATE_PLANNING, "construction: newly created worksite starts planning");
    failed += assert_true(game_construction_required_total(&worksite) == 4u, "construction: required total tracks work requirements");

    failed += assert_true(
        game_construction_update(
            &worksite_system,
            &board,
            &inventory,
            &haul_system,
            &structure_system,
            &map,
            2u,
            worksite_handle,
            NULL,
            &audit_log
        ) == GAME_CONSTRUCTION_RESULT_OK,
        "construction: update to start haul delivery"
    );
    failed += assert_true(
            game_construction_status(&worksite_system, worksite_handle, &worksite) == GAME_CONSTRUCTION_RESULT_OK
            && worksite.state == GAME_CONSTRUCTION_STATE_DELIVERING
            && worksite.active_haul.version != 0u,
        "construction: active haul is assigned"
    );

    GameHaulJob haul_status = {0};
    failed += assert_true(
        game_haul_job_status(&haul_system, worksite.active_haul, &haul_status) == GAME_HAUL_JOB_RESULT_OK,
        "construction: active haul is queryable"
    );

    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            worksite.active_haul,
            GAME_HAUL_JOB_STATE_RESERVE_SOURCE,
            3u,
            NULL,
            NULL
        ) == GAME_HAUL_JOB_RESULT_OK,
        "construction: advance haul to reserve source"
    );
    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            worksite.active_haul,
            GAME_HAUL_JOB_STATE_PICKUP,
            4u,
            NULL,
            NULL
        ) == GAME_HAUL_JOB_RESULT_OK,
        "construction: advance haul to pickup"
    );
    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            worksite.active_haul,
            GAME_HAUL_JOB_STATE_IN_TRANSIT,
            5u,
            NULL,
            NULL
        ) == GAME_HAUL_JOB_RESULT_OK,
        "construction: advance haul to in-transit"
    );
    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            worksite.active_haul,
            GAME_HAUL_JOB_STATE_DELIVERY,
            6u,
            NULL,
            NULL
        ) == GAME_HAUL_JOB_RESULT_OK,
        "construction: advance haul to delivery"
    );
    failed += assert_true(
        game_haul_job_advance(
            &haul_system,
            &board,
            &inventory,
            &path_service,
            worksite.active_haul,
            GAME_HAUL_JOB_STATE_DONE,
            7u,
            NULL,
            NULL
        ) == GAME_HAUL_JOB_RESULT_OK,
        "construction: complete haul delivery"
    );

    failed += assert_true(
        game_construction_update(
            &worksite_system,
            &board,
            &inventory,
            &haul_system,
            &structure_system,
            &map,
            8u,
            worksite_handle,
            NULL,
            &audit_log
        ) == GAME_CONSTRUCTION_RESULT_OK,
        "construction: settle completed delivery"
    );
    failed += assert_true(
        game_construction_status(&worksite_system, worksite_handle, &worksite) == GAME_CONSTRUCTION_RESULT_OK
            && worksite.state == GAME_CONSTRUCTION_STATE_PLANNING,
        "construction: return to planning after haul settles"
    );
    failed += assert_true(
        game_construction_delivered_total(&worksite) == 4u,
        "construction: delivered total reaches requirement"
    );

    failed += assert_true(
        game_construction_update(
            &worksite_system,
            &board,
            &inventory,
            &haul_system,
            &structure_system,
            &map,
            9u,
            worksite_handle,
            NULL,
            &audit_log
        ) == GAME_CONSTRUCTION_RESULT_OK,
        "construction: enter first build tick"
    );
    failed += assert_true(
        game_construction_status(&worksite_system, worksite_handle, &worksite) == GAME_CONSTRUCTION_RESULT_OK
            && worksite.state == GAME_CONSTRUCTION_STATE_BUILDING,
        "construction: worksite transitions to building"
    );
    failed += assert_true(
        game_construction_progress(&worksite_system, worksite_handle) == 50u,
        "construction: building progress updates"
    );

    failed += assert_true(
        game_construction_update(
            &worksite_system,
            &board,
            &inventory,
            &haul_system,
            &structure_system,
            &map,
            10u,
            worksite_handle,
            NULL,
            &audit_log
        ) == GAME_CONSTRUCTION_RESULT_OK,
        "construction: enter completion after required work ticks"
    );
    failed += assert_true(
        game_construction_status(&worksite_system, worksite_handle, &worksite) == GAME_CONSTRUCTION_RESULT_OK,
        "construction: re-query worksite after build completion"
    );
    failed += assert_true(worksite.state == GAME_CONSTRUCTION_STATE_DONE, "construction: construction worksite becomes done");

    GameJobOrder worksite_board_order = {0};
    failed += assert_true(
        game_job_board_status(&board, worksite.work_order, &worksite_board_order) == GAME_JOB_BOARD_RESULT_OK
            && worksite_board_order.state == GAME_JOB_BOARD_STATE_DONE,
        "construction: board order transitions to done"
    );

    int32_t placed_tile_value = 0;
    failed += assert_true(
        game_world_map_get(&map, target_tile, &placed_tile_value) == GAME_WORLD_MAP_RESULT_OK,
        "construction: placed tile is readable after completion"
    );
    failed += assert_true(
        game_structure_tile_has_structure(placed_tile_value),
        "construction: completion writes structure marker into world"
    );
    failed += assert_true(
        game_structure_tile_blocks_path(placed_tile_value),
        "construction: completion creates blocking terrain"
    );

    GameInventoryEntryView destination_query[8] = {0};
    size_t destination_count = game_inventory_dump_owner(&inventory, destination_owner, destination_query, 8u);
    failed += assert_true(
        destination_count == 1u && destination_query[0].resource_id == 7u && destination_query[0].total_quantity == 4u,
        "construction: destination receives hauled resources"
    );

    GameInventoryEntryView source_query[8] = {0};
    size_t source_count = game_inventory_dump_owner(&inventory, source_owner, source_query, 8u);
    failed += assert_true(
        source_count == 1u && source_query[0].resource_id == 7u && source_query[0].total_quantity == 2u,
        "construction: source quantity decreases by delivered amount"
    );

    failed += assert_true(audit_log.count >= 5u, "construction: audit trail captured workflow events");
    failed += assert_true(
        audit_log.entries[audit_log.count - 1u].reason == GAME_CONSTRUCTION_AUDIT_COMPLETED,
        "construction: final audit reason is completed"
    );

    game_world_map_destroy(&map);
    return failed;
}

static int test_construction_abort_releases_workflow(void) {
    int failed = 0;

    GameInventoryEntry inv_entries[8] = {0};
    GameInventoryReservation inv_reservations[8] = {0};
    GameInventory inventory = {0};
    failed += assert_true(
        game_inventory_init(&inventory, inv_entries, 8u, inv_reservations, 8u, 12u) == GAME_INVENTORY_RESULT_OK,
        "construction abort: init inventory"
    );

    GameInventoryOwner source_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {51u, 1u}, 0u};
    GameInventoryOwner destination_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {52u, 1u}, 0u};
    failed += assert_true(
        game_inventory_add(&inventory, source_owner, 9u, 3u, NULL) == GAME_INVENTORY_RESULT_OK,
        "construction abort: load source resources"
    );

    GameJobBoard board = {0};
    GameJobBoardOrderSlot board_slots[4] = {0};
    failed += assert_true(
        game_job_board_init(&board, board_slots, 4u, 4u) == GAME_JOB_BOARD_RESULT_OK,
        "construction abort: init job board"
    );

    GameHaulJobSystem haul_system = {0};
    GameHaulJob haul_slots[2] = {0};
    failed += assert_true(
        game_haul_job_init(&haul_system, haul_slots, 2u) == GAME_HAUL_JOB_RESULT_OK,
        "construction abort: init haul job system"
    );

    GameConstructionSystem worksite_system = {0};
    GameConstructionWorksite worksite_slots[2] = {0};
    failed += assert_true(
        game_construction_init(&worksite_system, worksite_slots, 2u, 2u) == GAME_CONSTRUCTION_RESULT_OK,
        "construction abort: init construction system"
    );

    GameWorldMap map = {0};
    failed += assert_true(
        game_world_map_init(&map, 4, -1) == GAME_WORLD_MAP_RESULT_OK,
        "construction abort: init map"
    );
    GameHexAxial target_tile = {3, 3};
    failed += assert_true(
        game_world_map_create_chunk(&map, game_hex_cube_to_chunk_key(game_hex_axial_to_cube(target_tile), 4))
            == GAME_WORLD_MAP_RESULT_OK,
        "construction abort: create chunk"
    );

    static const GameStructureFootprintOffset foundation[1] = {{0, 0}};
    static const GameStructureDefinition defs[] = {
        {102u, GAME_STRUCTURE_TILE_FLAG_BLOCKS_PATH, GAME_STRUCTURE_FIELD_FLAG_NONE, 1u, foundation},
    };
    GameStructurePlacement structure_placements[2] = {0};
    GameStructureSystem structure_system = {0};
    failed += assert_true(
        game_structure_init(&structure_system, defs, 1u, structure_placements, 2u, 2u) == GAME_STRUCTURE_RESULT_OK,
        "construction abort: init structure system"
    );

    uint16_t path_costs[2u] = {1u, 1u};
    GamePathCostMap path_cost_map = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 1u, .r_count = 2u},
        .cost = path_costs,
        .blocked_cost = UINT16_MAX,
    };
    uint32_t g[2u] = {0u, 0u};
    uint32_t f[2u] = {0u, 0u};
    int32_t parent[2u] = {-1, -1};
    bool open[2u] = {0};
    bool closed[2u] = {0};
    GamePathQueryScratch path_scratch = {
        .capacity = 2u,
        .g_score = g,
        .f_score = f,
        .parent = parent,
        .open = open,
        .closed = closed,
    };
    GamePathService path_service = {0};
    GamePathServiceRequestSlot path_slots[1u] = {0};
    failed += assert_true(
        game_path_service_init(&path_service, path_slots, 1u, 3u) == GAME_PATH_SERVICE_RESULT_OK,
        "construction abort: init path service"
    );
    GameHexAxial path_buffer[8] = {0};
    GamePathRequestHandle route = {0u, 0u};
    failed += assert_true(
        game_path_service_submit(
            &path_service,
            0u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 1},
            &path_cost_map,
            &path_scratch,
            2u,
            path_buffer,
            8u,
            &route
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "construction abort: submit route"
    );
    failed += assert_true(
        game_path_service_advance_tick(&path_service, 1u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "construction abort: resolve route"
    );

    GameConstructionAuditEntry audit_entries[8] = {0};
    GameConstructionAuditLog audit_log = {0};
    game_construction_audit_init(&audit_log, audit_entries, 8u);

    GameConstructionResourceRequirement reqs[1] = {{9u, 2u, 0u}};
    GameConstructionCreateInfo create_info = {
        .target_tile = target_tile,
        .structure_definition_id = 102u,
        .assigned_worker = (GameEntityId){101u, 1u},
        .resource_source_owner = source_owner,
        .resource_destination_owner = destination_owner,
        .required_work_ticks = 1u,
        .worker_capacity = 2u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .required_resource_flags = 0u,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
        .haul_route_request = route,
        .requirements = reqs,
        .requirement_count = 1u,
    };

    GameConstructionWorksiteHandle worksite_handle = {0u, 0u};
    failed += assert_true(
        game_construction_create(
            &worksite_system,
            &board,
            &inventory,
            2u,
            &create_info,
            &worksite_handle
        ) == GAME_CONSTRUCTION_RESULT_OK,
        "construction abort: create abortable worksite"
    );

    failed += assert_true(
        game_construction_update(
            &worksite_system,
            &board,
            &inventory,
            &haul_system,
            &structure_system,
            &map,
            3u,
            worksite_handle,
            NULL,
            &audit_log
        ) == GAME_CONSTRUCTION_RESULT_OK,
        "construction abort: request delivery for active work"
    );

    GameConstructionWorksite worksite = {0};
    failed += assert_true(
        game_construction_status(&worksite_system, worksite_handle, &worksite) == GAME_CONSTRUCTION_RESULT_OK,
        "construction abort: query active worksite"
    );
    failed += assert_true(worksite.state == GAME_CONSTRUCTION_STATE_DELIVERING, "construction abort: state reaches delivering");

    GameHaulJobHandle active_haul = worksite.active_haul;
    failed += assert_true(
        active_haul.version != 0u,
        "construction abort: has active haul before abort"
    );

    failed += assert_true(
        game_construction_abort(
            &worksite_system,
            &board,
            &haul_system,
            &inventory,
            worksite_handle,
            4u,
            NULL,
            &audit_log
        ) == GAME_CONSTRUCTION_RESULT_OK,
        "construction abort: abort from delivering state"
    );

    failed += assert_true(
        game_construction_status(&worksite_system, worksite_handle, &worksite) == GAME_CONSTRUCTION_RESULT_OK
            && worksite.state == GAME_CONSTRUCTION_STATE_ABORTED,
        "construction abort: worksite state becomes aborted"
    );
    failed += assert_true(
        worksite.active_haul.slot == 0u && worksite.active_haul.version == 0u,
        "construction abort: active haul handle cleared"
    );

    GameHaulJob hauled_job = {0};
    failed += assert_true(
        game_haul_job_status(&haul_system, active_haul, &hauled_job) == GAME_HAUL_JOB_RESULT_OK
            && hauled_job.state == GAME_HAUL_JOB_STATE_ABORTED,
        "construction abort: haul state becomes aborted"
    );

    GameJobOrder board_order = {0};
    failed += assert_true(
        game_job_board_status(&board, worksite.work_order, &board_order) == GAME_JOB_BOARD_RESULT_OK,
        "construction abort: workboard order remains visible"
    );
    failed += assert_true(
        board_order.state == GAME_JOB_BOARD_STATE_ABORTED,
        "construction abort: board order transitions to aborted"
    );

    GameInventoryEntryView source_after[8] = {0};
    size_t source_after_count = game_inventory_dump_owner(&inventory, source_owner, source_after, 8u);
    failed += assert_true(
        source_after_count == 1u && source_after[0].resource_id == 9u && source_after[0].total_quantity == 3u,
        "construction abort: source inventory remains untouched"
    );

    GameInventoryEntryView destination_after[8] = {0};
    size_t destination_after_count = game_inventory_dump_owner(&inventory, destination_owner, destination_after, 8u);
    failed += assert_true(destination_after_count == 0u, "construction abort: destination stays empty");

    failed += assert_true(
        audit_log.count >= 1u && audit_log.entries[audit_log.count - 1u].reason == GAME_CONSTRUCTION_AUDIT_ABORTED,
        "construction abort: audit records aborted state"
    );

    game_world_map_destroy(&map);
    return failed;
}

int test_construction(void) {
    int failed = 0;
    failed += test_construction_completion_flow();
    failed += test_construction_abort_releases_workflow();

    if (failed == 0) {
        printf("[construction] PASS\n");
    }
    return failed;
}
