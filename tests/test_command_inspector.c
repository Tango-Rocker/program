#include <stdio.h>

#include "ecs/entity.h"
#include "sim/command.h"
#include "ui/command_inspector.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[command_inspector] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_command_inspector(void) {
    int failed = 0;
    GameCommandInspector inspector = {0};
    game_command_inspector_init(&inspector);

    GameEntityRegistry registry = {0};
    failed += assert_true(game_entity_registry_init(&registry, 2u) == GAME_ENTITY_RESULT_OK, "registry init");
    GameEntityId actor = {0};
    failed += assert_true(game_entity_registry_create(&registry, &actor) == GAME_ENTITY_RESULT_OK, "actor create");

    GameCommandQueue queue = {0};
    GameCommandQueueConfig queue_config = {.capacity = 2u, .accepted_tick_window = 4u};
    failed += assert_true(game_command_queue_init(&queue, &queue_config) == GAME_COMMAND_QUEUE_RESULT_OK, "queue init");

    GameCommandNoisePayload payload = {
        .origin_q = 0,
        .origin_r = 0,
        .intensity = 4,
        .max_radius = 1,
        .attenuation = 1,
    };
    failed += assert_true(
        game_command_inspector_stage_noise(&inspector, actor, 1u, &payload) == GAME_COMMAND_QUEUE_RESULT_OK,
        "stage noise"
    );
    failed += assert_true(game_command_queue_count(&queue) == 0u, "staging does not submit");
    failed += assert_true(
        game_command_inspector_validate(&inspector, 0u, &registry) == GAME_COMMAND_QUEUE_RESULT_OK,
        "valid staged command"
    );
    failed += assert_true(
        game_command_inspector_submit(&inspector, &queue, 0u, &registry) == GAME_COMMAND_QUEUE_RESULT_OK,
        "submit staged command"
    );
    failed += assert_true(game_command_queue_count(&queue) == 1u, "submit queues command");

    failed += assert_true(
        game_command_inspector_stage_noise(&inspector, (GameEntityId){99u, 99u}, 1u, &payload)
            == GAME_COMMAND_QUEUE_RESULT_OK,
        "stage invalid source"
    );
    failed += assert_true(
        game_command_inspector_submit(&inspector, &queue, 0u, &registry) == GAME_COMMAND_QUEUE_RESULT_INVALID_ARGUMENT,
        "invalid submit rejected"
    );
    failed += assert_true(game_command_queue_count(&queue) == 1u, "invalid submit does not mutate queue");
    failed += assert_true(
        game_command_inspector_result_message(inspector.last_result)[0] != '\0',
        "validation message inspectable"
    );

    game_command_queue_destroy(&queue);
    game_entity_registry_destroy(&registry);
    if (failed == 0) {
        printf("[command_inspector] PASS\n");
    }
    return failed;
}
