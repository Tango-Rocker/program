#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "nav/path_service.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[path_service] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_path_service(void) {
    int failed = 0;

    GamePathService service = {0};
    GamePathServiceRequestSlot slots[4] = {0};
    failed += assert_true(
        game_path_service_init(&service, slots, 4u, 2u) == GAME_PATH_SERVICE_RESULT_OK,
        "path service init"
    );

    uint16_t map[5] = {1, 1, 1, 1, 1};
    GamePathCostMap line = {
        .grid = {.q_min = 0, .r_min = 0, .q_count = 1u, .r_count = 5u},
        .cost = map,
        .blocked_cost = UINT16_MAX,
    };

    uint32_t g1[5] = {0};
    uint32_t f1[5] = {0};
    int32_t parent1[5] = {0};
    bool open1[5] = {0};
    bool closed1[5] = {0};
    GamePathQueryScratch scratch1 = {
        .capacity = 5u,
        .g_score = g1,
        .f_score = f1,
        .parent = parent1,
        .open = open1,
        .closed = closed1,
    };

    GameHexAxial result_storage[16];
    GamePathRequestHandle request1 = {0};
    failed += assert_true(
        game_path_service_submit(
            &service,
            0u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 4},
            &line,
            &scratch1,
            10u,
            result_storage,
            16u,
            &request1
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "submit path request"
    );

    GamePathServiceStatus status = {0};
    failed += assert_true(
        game_path_service_status(&service, request1, &status) == GAME_PATH_SERVICE_RESULT_OK,
        "status for submitted request"
    );
    failed += assert_true(status.state == GAME_PATH_SERVICE_STATE_PENDING, "request is pending");

    failed += assert_true(
        game_path_service_advance_tick(&service, 1u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "advance tick resolves request"
    );
    failed += assert_true(
        game_path_service_status(&service, request1, &status) == GAME_PATH_SERVICE_RESULT_OK,
        "status after resolve tick"
    );
    failed += assert_true(status.state == GAME_PATH_SERVICE_STATE_RESOLVED, "request resolved");

    GameHexAxial path_out[16];
    size_t path_length = 0u;
    uint64_t resolved_version = 0u;
    failed += assert_true(
        game_path_service_retrieve(&service, request1, path_out, 16u, &path_length, &resolved_version) == GAME_PATH_SERVICE_RESULT_OK,
        "retrieve resolved path"
    );
    failed += assert_true(path_length == 5u, "resolved path length");
    failed += assert_true(
        resolved_version == status.result_version && status.state == GAME_PATH_SERVICE_STATE_RESOLVED,
        "result version matches status"
    );

    GameHexAxial cancel_buffer[16];
    GamePathRequestHandle request2 = {0};
    failed += assert_true(
        game_path_service_submit(
            &service,
            2u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 4},
            &line,
            &scratch1,
            10u,
            cancel_buffer,
            16u,
            &request2
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "submit second request for cancellation"
    );
    failed += assert_true(
        game_path_service_cancel(&service, request2) == GAME_PATH_SERVICE_RESULT_OK,
        "cancel pending request"
    );
    failed += assert_true(
        game_path_service_status(&service, request2, &status) == GAME_PATH_SERVICE_RESULT_OK,
        "status for cancelled request"
    );
    failed += assert_true(status.state == GAME_PATH_SERVICE_STATE_CANCELLED, "request cancelled");

    GameHexAxial cancel_path[16];
    size_t cancel_length = 0u;
    uint64_t cancel_version = 0u;
    failed += assert_true(
        game_path_service_retrieve(&service, request2, cancel_path, 16u, &cancel_length, &cancel_version) != GAME_PATH_SERVICE_RESULT_OK,
        "cannot retrieve cancelled request"
    );

    GameHexAxial expire_buffer[16];
    GamePathRequestHandle request3 = {0};
    failed += assert_true(
        game_path_service_submit(
            &service,
            3u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 4},
            &line,
            &scratch1,
            10u,
            expire_buffer,
            16u,
            &request3
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "submit request for expiration test"
    );
    failed += assert_true(
        game_path_service_advance_tick(&service, 3u, 0u) == GAME_PATH_SERVICE_RESULT_OK,
        "advance same tick with no budget"
    );
    failed += assert_true(
        game_path_service_advance_tick(&service, 4u, 0u) == GAME_PATH_SERVICE_RESULT_OK,
        "advance before expiry without resolving"
    );
    failed += assert_true(
        game_path_service_advance_tick(&service, 5u, 0u) == GAME_PATH_SERVICE_RESULT_OK,
        "advance into expiry"
    );
    failed += assert_true(
        game_path_service_status(&service, request3, &status) == GAME_PATH_SERVICE_RESULT_OK
            && status.state == GAME_PATH_SERVICE_STATE_EXPIRED,
        "expired request visible"
    );

    failed += assert_true(
        game_path_service_advance_tick(&service, 6u, 0u) == GAME_PATH_SERVICE_RESULT_OK,
        "advance cleanup tick"
    );
    failed += assert_true(
        game_path_service_status(&service, request3, &status) == GAME_PATH_SERVICE_RESULT_INVALID_HANDLE,
        "expired request handle becomes stale"
    );

    uint32_t g2[5] = {0};
    uint32_t f2[5] = {0};
    int32_t parent2[5] = {0};
    bool open2[5] = {0};
    bool closed2[5] = {0};
    GamePathQueryScratch scratch2 = {
        .capacity = 5u,
        .g_score = g2,
        .f_score = f2,
        .parent = parent2,
        .open = open2,
        .closed = closed2,
    };

    uint32_t g3[5] = {0};
    uint32_t f3[5] = {0};
    int32_t parent3[5] = {0};
    bool open3[5] = {0};
    bool closed3[5] = {0};
    GamePathQueryScratch scratch3 = {
        .capacity = 5u,
        .g_score = g3,
        .f_score = f3,
        .parent = parent3,
        .open = open3,
        .closed = closed3,
    };

    GameHexAxial budget_buffer_a[16];
    GameHexAxial budget_buffer_b[16];
    GamePathRequestHandle request4 = {0};
    GamePathRequestHandle request5 = {0};
    failed += assert_true(
        game_path_service_submit(
            &service,
            10u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 4},
            &line,
            &scratch2,
            1u,
            budget_buffer_a,
            16u,
            &request4
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "submit budgeted request A"
    );
    failed += assert_true(
        game_path_service_submit(
            &service,
            10u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 4},
            &line,
            &scratch3,
            1u,
            budget_buffer_b,
            16u,
            &request5
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "submit budgeted request B"
    );
    failed += assert_true(
        game_path_service_advance_tick(&service, 10u, 1u) == GAME_PATH_SERVICE_RESULT_OK,
        "tick budget can resolve one request"
    );
    failed += assert_true(
        game_path_service_status(&service, request4, &status) == GAME_PATH_SERVICE_RESULT_OK
            && status.state == GAME_PATH_SERVICE_STATE_RESOLVED,
        "request A resolves first"
    );
    failed += assert_true(
        game_path_service_status(&service, request5, &status) == GAME_PATH_SERVICE_RESULT_OK
            && status.state == GAME_PATH_SERVICE_STATE_PENDING,
        "request B remains pending"
    );

    failed += assert_true(
        game_path_service_advance_tick(&service, 11u, 1u) == GAME_PATH_SERVICE_RESULT_OK,
        "next tick budget resolves second request"
    );
    failed += assert_true(
        game_path_service_status(&service, request5, &status) == GAME_PATH_SERVICE_RESULT_OK
            && status.state == GAME_PATH_SERVICE_STATE_RESOLVED,
        "request B resolves second"
    );

    GamePathService reuse_service = {0};
    GamePathServiceRequestSlot reuse_slot[1] = {0};
    uint32_t g4[5] = {0};
    uint32_t f4[5] = {0};
    int32_t parent4[5] = {0};
    bool open4[5] = {0};
    bool closed4[5] = {0};
    GamePathQueryScratch reuse_scratch = {
        .capacity = 5u,
        .g_score = g4,
        .f_score = f4,
        .parent = parent4,
        .open = open4,
        .closed = closed4,
    };

    failed += assert_true(
        game_path_service_init(&reuse_service, reuse_slot, 1u, 2u) == GAME_PATH_SERVICE_RESULT_OK,
        "init reuse service"
    );

    GamePathRequestHandle first_reuse = {0};
    GameHexAxial reuse_path_a[16] = {0};
    failed += assert_true(
        game_path_service_submit(
            &reuse_service,
            0u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 4},
            &line,
            &reuse_scratch,
            10u,
            reuse_path_a,
            16u,
            &first_reuse
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "submit first reuse request"
    );
    failed += assert_true(
        game_path_service_advance_tick(&reuse_service, 1u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "resolve first reuse request"
    );
    failed += assert_true(
        game_path_service_status(&reuse_service, first_reuse, &status) == GAME_PATH_SERVICE_RESULT_OK
            && status.state == GAME_PATH_SERVICE_STATE_RESOLVED,
        "first reuse request resolved"
    );

    GamePathRequestHandle second_reuse = {0};
    GameHexAxial reuse_path_b[16] = {0};
    failed += assert_true(
        game_path_service_advance_tick(&reuse_service, 2u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "advance reuse service to expire old request"
    );
    failed += assert_true(
        game_path_service_advance_tick(&reuse_service, 3u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "cleanup old reuse request"
    );
    failed += assert_true(
        game_path_service_submit(
            &reuse_service,
            3u,
            (GameHexAxial){0, 0},
            (GameHexAxial){0, 4},
            &line,
            &reuse_scratch,
            10u,
            reuse_path_b,
            16u,
            &second_reuse
        ) == GAME_PATH_SERVICE_RESULT_OK,
        "submit reused slot request"
    );
    failed += assert_true(
        second_reuse.version != first_reuse.version,
        "reused handle version is newer"
    );
    failed += assert_true(
        game_path_service_advance_tick(&reuse_service, 4u, 10u) == GAME_PATH_SERVICE_RESULT_OK,
        "resolve reused request"
    );
    failed += assert_true(
        game_path_service_status(&reuse_service, second_reuse, &status) == GAME_PATH_SERVICE_RESULT_OK
            && status.state == GAME_PATH_SERVICE_STATE_RESOLVED,
        "reused request resolves"
    );
    failed += assert_true(
        game_path_service_status(&reuse_service, first_reuse, &status) == GAME_PATH_SERVICE_RESULT_INVALID_HANDLE,
        "old handle is stale after slot reuse"
    );

    if (failed == 0) {
        printf("[path_service] PASS\n");
    }
    return failed;
}
