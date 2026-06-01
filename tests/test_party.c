#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "event/event_log.h"
#include "sim/party.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[party] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_party(void) {
    int failed = 0;

    GamePartySlot slots[3] = {0};
    GameParty party = {0};
    GamePartyActorState actors[3] = {
        {.id = {1u, 1u}, .position = {0, 0}, .health = 10, .faction_id = 2u, .selectable = true},
        {.id = {2u, 1u}, .position = {1, 0}, .health = 10, .faction_id = 2u, .selectable = true},
        {.id = {3u, 1u}, .position = {2, 0}, .health = 10, .faction_id = 3u, .selectable = true},
    };

    game_party_init(&party, slots, 3u);
    GamePartyActorHandle first = {0u, 0u};
    GamePartyActorHandle second = {0u, 0u};
    GamePartyActorHandle third = {0u, 0u};

    failed += assert_true(game_party_register_actor(&party, &actors[0], &first) == GAME_PARTY_RESULT_OK, "register first actor");
    failed += assert_true(game_party_register_actor(&party, &actors[1], &second) == GAME_PARTY_RESULT_OK, "register second actor");
    failed += assert_true(game_party_register_actor(&party, &actors[2], &third) == GAME_PARTY_RESULT_OK, "register third actor");
    failed += assert_true(game_party_count(&party) == 3u, "registered actors count");

    GameEventLog log = {0};
    failed += assert_true(game_event_log_init(&log, 8u) == GAME_EVENT_LOG_RESULT_OK, "event log init");

    GamePartyActorState selected_actor = {0};
    failed += assert_true(game_party_get_actor(&party, first, &selected_actor) == GAME_PARTY_RESULT_OK, "lookup first actor");
    failed += assert_true(selected_actor.id.index == 1u, "first actor id preserved");

    GamePartySelectionEventPayload payload1 = {0};
    failed += assert_true(
        game_party_select_actor(&party, first, 10u, &log, &payload1) == GAME_PARTY_RESULT_OK,
        "select first actor"
    );
    failed += assert_true(game_party_has_selection(&party), "selection state recorded");
    failed += assert_true(game_party_selected_handle(&party).slot == first.slot, "selected handle is first");
    failed += assert_true(payload1.previous.slot == 0u && payload1.previous.generation == 0u, "previous handle empty");

    failed += assert_true(game_party_next_selectable(&party, 11u, &log, &second) == GAME_PARTY_RESULT_OK, "select next selectable");
    failed += assert_true(second.slot == 1u, "next selection is deterministic second");

    GamePartySelectionEventPayload payload_stale = {0};
    GamePartyActorHandle stale_handle = first;
    stale_handle.generation = first.generation + 1u;
    failed += assert_true(
        game_party_select_actor(&party, stale_handle, 12u, &log, &payload_stale) == GAME_PARTY_RESULT_STALE_HANDLE,
        "stale actor handle rejected"
    );

    failed += assert_true(game_party_clear_selection(&party, 13u, &log) == GAME_PARTY_RESULT_OK, "clear selection");
    failed += assert_true(!game_party_has_selection(&party), "selection cleared");
    failed += assert_true(game_party_selected_handle(&party).slot == 0u && game_party_selected_handle(&party).generation == 0u, "clear resets handle");

    failed += assert_true(game_event_log_count(&log) == 3u, "selection writes trace entries");
    const GameEventLogEntry *first_log = game_event_log_at(&log, 0u);
    const GameEventLogEntry *second_log = game_event_log_at(&log, 1u);
    failed += assert_true(first_log != NULL && first_log->sequence == 1u, "selection trace sequence first");
    failed += assert_true(second_log != NULL && second_log->sequence == 2u, "selection trace sequence second");
    failed += assert_true(second_log != NULL && second_log->type == GAME_EVENT_TYPE_PARTY_SELECTED, "selection trace type");

    game_event_log_destroy(&log);

    if (failed == 0) {
        printf("[party] PASS\n");
    }
    return failed;
}
