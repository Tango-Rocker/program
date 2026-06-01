#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ecs/entity.h"
#include "sim/command.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[command] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_command_queue(void) {
    int failed = 0;
    GameEntityRegistry registry = {0};
    if (game_entity_registry_init(&registry, 4) != GAME_ENTITY_RESULT_OK) {
        return 1;
    }

    GameEntityId player = {0, 0};
    if (game_entity_registry_create(&registry, &player) != GAME_ENTITY_RESULT_OK) {
        failed += assert_true(0, "entity create for command sender");
    }

    GameCommandQueue queue = {0};
    GameCommandQueueConfig cfg = { .capacity = 2u, .accepted_tick_window = 4u };
    failed += assert_true(game_command_queue_init(&queue, &cfg) == GAME_COMMAND_QUEUE_RESULT_OK, "command queue init");

    GameCommandNoisePayload noise = {
        .origin_q = 1,
        .origin_r = -1,
        .intensity = 16,
        .max_radius = 3,
        .attenuation = 1,
        .decay_ticks = 2,
    };
    GameCommand c1 = {0};
    c1.type = GAME_COMMAND_TYPE_EMIT_NOISE;
    c1.requested_tick = 10;
    c1.source = player;
    c1.payload_size = sizeof(GameCommandNoisePayload);
    memcpy(c1.payload.bytes, &noise, sizeof(noise));

    failed += assert_true(game_command_queue_push(&queue, &c1, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_OK, "push valid command");
    failed += assert_true(game_command_queue_count(&queue) == 1u, "command count after one push");
    failed += assert_true(game_command_queue_push(&queue, &c1, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_OK, "push second command");

    GameCommand first = {0};
    GameCommand second = {0};
    failed += assert_true(game_command_queue_pop(&queue, &first) == GAME_COMMAND_QUEUE_RESULT_OK, "pop first");
    failed += assert_true(first.requested_tick == 10u, "fifo first tick");
    failed += assert_true(game_command_queue_pop(&queue, &second) == GAME_COMMAND_QUEUE_RESULT_OK, "pop second");
    failed += assert_true(second.requested_tick == 10u, "fifo second tick");

    GameCommand stale = c1;
    stale.requested_tick = 9u;
    failed += assert_true(game_command_queue_push(&queue, &stale, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_STALE_TICK, "reject stale tick");

    GameCommand future = c1;
    future.requested_tick = 20u;
    failed += assert_true(game_command_queue_push(&queue, &future, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_OUTSIDE_WINDOW, "reject outside window");

    GameCommand bad = c1;
    bad.type = (GameCommandType)1234;
    failed += assert_true(game_command_queue_push(&queue, &bad, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_UNSUPPORTED_COMMAND, "reject unsupported type");

    GameCommand bad_payload = c1;
    bad_payload.payload_size = GAME_COMMAND_MAX_PAYLOAD_BYTES + 1u;
    failed += assert_true(game_command_queue_push(&queue, &bad_payload, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD, "reject bad payload");

    GameCommand invalid_source = c1;
    invalid_source.source = (GameEntityId){1u, 0u};
    failed += assert_true(game_command_queue_push(&queue, &invalid_source, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT, "reject invalid source");

    GameCommandQueueResult full = GAME_COMMAND_QUEUE_RESULT_OK;
    failed += assert_true(game_command_queue_push(&queue, &c1, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_OK, "queue reusable push one");
    failed += assert_true(game_command_queue_push(&queue, &c1, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_OK, "queue reusable push two");
    failed += assert_true(game_command_queue_push(&queue, &c1, 10u, &registry) == GAME_COMMAND_QUEUE_RESULT_FULL, "reject overflow");

    game_command_queue_destroy(&queue);
    game_entity_registry_destroy(&registry);

    if (failed == 0) {
        printf("[command] PASS\n");
    }
    return failed;
}
