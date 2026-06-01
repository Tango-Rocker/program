#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "event/event_log.h"
#include "event/event_queue.h"
#include "sim/combat.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[combat] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_combat_ability(void) {
    int failed = 0;

    GameCombatAbility abilities[4] = {0};
    GameCombatAbilityCatalog catalog = {0};
    failed += assert_true(game_combat_init_catalog(&catalog, abilities, 4u) == GAME_COMBAT_RESULT_OK, "combat catalog init");
    failed += assert_true(
        game_combat_catalog_set(&catalog, 10u, 2u, 3u, 4) == GAME_COMBAT_RESULT_OK,
        "catalog set ability"
    );

    GameCombatCooldownSlot cooldown_slots[4] = {0};
    GameCombatCooldownTable cooldown_table = {0};
    failed += assert_true(game_combat_cooldown_init(&cooldown_table, cooldown_slots, 4u) == GAME_COMBAT_RESULT_OK, "cooldown init");

    GameCombatAbilityCommand command = {
        .ability_id = 10u,
        .actor = {1u, 2u},
        .target = {2u, 3u},
        .actor_position = {0, 0},
        .target_position = {1, 0},
    };

    GameEventQueue event_queue = {0};
    failed += assert_true(game_event_queue_init(&event_queue, 4u) == GAME_EVENT_QUEUE_RESULT_OK, "event queue init");
    GameEventLog event_log = {0};
    failed += assert_true(game_event_log_init(&event_log, 4u) == GAME_EVENT_LOG_RESULT_OK, "event log init");

    GameCombatResultData result = {0};
    failed += assert_true(
        game_combat_resolve_ability(&catalog, &cooldown_table, &command, 0u, &event_queue, &event_log, &result) == GAME_COMBAT_RESULT_OK,
        "valid ability use"
    );
    failed += assert_true(result.cooldown_next_tick == 3u, "cooldown applies");
    failed += assert_true(result.ability_event_sequence == 1u, "ability event sequence");

    failed += assert_true(
        game_combat_resolve_ability(&catalog, &cooldown_table, &command, 1u, NULL, &event_log, &result) == GAME_COMBAT_RESULT_COOLDOWN,
        "cooldown rejection"
    );

    GameCombatAbilityCommand too_far = command;
    too_far.target_position = (GameHexAxial){5, 0};
    failed += assert_true(
        game_combat_resolve_ability(&catalog, &cooldown_table, &too_far, 4u, NULL, &event_log, &result) == GAME_COMBAT_RESULT_OUT_OF_RANGE,
        "out of range rejection"
    );

    GameCombatAbilityCommand invalid_actor = command;
    invalid_actor.actor = (GameEntityId){UINT32_MAX, UINT32_MAX};
    failed += assert_true(
        game_combat_resolve_ability(&catalog, &cooldown_table, &invalid_actor, 4u, NULL, &event_log, &result) == GAME_COMBAT_RESULT_INVALID_HANDLE,
        "invalid actor rejected"
    );

    GameEvent queued_event = {0};
    failed += assert_true(game_event_queue_peek(&event_queue, &queued_event) == GAME_EVENT_QUEUE_RESULT_OK, "ability emit queueed");
    failed += assert_true(queued_event.type == GAME_EVENT_TYPE_ABILITY_USED, "ability emit type");
    GameCombatCooldownSlot cooldown_slots_trace[4] = {0};
    GameCombatCooldownTable cooldown_trace = {0};
    failed += assert_true(game_combat_cooldown_init(&cooldown_trace, cooldown_slots_trace, 4u) == GAME_COMBAT_RESULT_OK, "trace cooldown init");
    GameEventQueue trace_queue = {0};
    failed += assert_true(game_event_queue_init(&trace_queue, 2u) == GAME_EVENT_QUEUE_RESULT_OK, "trace event queue init");
    GameEventLog trace_log = {0};
    failed += assert_true(game_event_log_init(&trace_log, 2u) == GAME_EVENT_LOG_RESULT_OK, "trace event log init");

    GameCombatResultData first = {0};
    GameCombatResultData second = {0};
    failed += assert_true(
        game_combat_resolve_ability(&catalog, &cooldown_trace, &command, 0u, &trace_queue, &trace_log, &first)
            == GAME_COMBAT_RESULT_OK,
        "trace run first use"
    );
    failed += assert_true(
        game_combat_resolve_ability(&catalog, &cooldown_trace, &command, 4u, &trace_queue, &trace_log, &second)
            == GAME_COMBAT_RESULT_OK,
        "trace run second use"
    );
    failed += assert_true(first.ability_event_sequence == 1u, "first trace seq");
    failed += assert_true(second.ability_event_sequence == 2u, "second trace seq");
    failed += assert_true(game_event_log_count(&trace_log) == 2u, "trace log count");

    game_event_queue_destroy(&trace_queue);
    game_event_log_destroy(&trace_log);

    game_event_queue_destroy(&event_queue);
    game_event_log_destroy(&event_log);

    if (failed == 0) {
        printf("[combat] PASS\n");
    }
    return failed;
}
