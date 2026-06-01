#include "sim/party.h"

#include <string.h>

static bool game_party_handle_matches(const GamePartySlot *slot, GamePartyActorHandle handle) {
    return slot && slot->in_use && slot->handle.slot == handle.slot && slot->handle.generation == handle.generation;
}

static GamePartyResult game_party_find_slot(const GameParty *party, GamePartyActorHandle handle, size_t *out_slot) {
    if (!party || !party->slots || handle.slot >= (uint32_t)party->capacity) {
        return GAME_PARTY_RESULT_INVALID_ARGUMENT;
    }

    const GamePartySlot *slot = &party->slots[handle.slot];
    if (!game_party_handle_matches(slot, handle)) {
        return GAME_PARTY_RESULT_STALE_HANDLE;
    }

    if (out_slot) {
        *out_slot = (size_t)handle.slot;
    }
    return GAME_PARTY_RESULT_OK;
}

void game_party_init(GameParty *party, GamePartySlot *slots, size_t capacity) {
    if (!party || !slots || capacity == 0u) {
        return;
    }

    party->slots = slots;
    party->capacity = capacity;
    party->selected_actor = (GamePartyActorHandle){0u, 0u};
    party->has_selection = false;
    party->last_selection_tick = 0u;

    for (size_t index = 0u; index < capacity; ++index) {
        slots[index] = (GamePartySlot){
            .handle = {(uint32_t)index, 1u},
            .generation = 1u,
            .in_use = false,
            .alive = false,
            .state = {0},
        };
    }
}

GamePartyResult game_party_register_actor(
    GameParty *party,
    const GamePartyActorState *state,
    GamePartyActorHandle *out_handle
) {
    if (!party || !state || !out_handle || !party->slots || party->capacity == 0u) {
        return GAME_PARTY_RESULT_INVALID_ARGUMENT;
    }

    for (size_t index = 0u; index < party->capacity; ++index) {
        GamePartySlot *slot = &party->slots[index];
        if (slot->in_use) {
            continue;
        }

        slot->in_use = true;
        slot->alive = true;
        slot->state = *state;
        slot->state.health = state->health < 0 ? 0 : state->health;
        slot->handle.slot = (uint32_t)index;
        slot->handle.generation = slot->generation;
        *out_handle = slot->handle;
        return GAME_PARTY_RESULT_OK;
    }

    return GAME_PARTY_RESULT_BUFFER_TOO_SMALL;
}

bool game_party_is_valid_handle(const GameParty *party, GamePartyActorHandle handle) {
    return game_party_find_slot(party, handle, NULL) == GAME_PARTY_RESULT_OK;
}

GamePartyResult game_party_get_actor(const GameParty *party, GamePartyActorHandle handle, GamePartyActorState *out_state) {
    if (!party || !out_state || !party->slots || handle.slot >= (uint32_t)party->capacity) {
        return GAME_PARTY_RESULT_INVALID_ARGUMENT;
    }

    const GamePartySlot *slot = &party->slots[handle.slot];
    if (!game_party_handle_matches(slot, handle) || !slot->alive) {
        return GAME_PARTY_RESULT_STALE_HANDLE;
    }

    *out_state = slot->state;
    return GAME_PARTY_RESULT_OK;
}

GamePartyResult game_party_set_selectable(GameParty *party, GamePartyActorHandle handle, bool selectable) {
    if (!party || !party->slots || handle.slot >= (uint32_t)party->capacity) {
        return GAME_PARTY_RESULT_INVALID_ARGUMENT;
    }

    GamePartySlot *slot = &party->slots[handle.slot];
    if (!game_party_handle_matches(slot, handle) || !slot->alive) {
        return GAME_PARTY_RESULT_INVALID_HANDLE;
    }

    slot->state.selectable = selectable;
    return GAME_PARTY_RESULT_OK;
}

static void game_party_clear_slot_selection(GameParty *party) {
    party->has_selection = false;
    party->selected_actor = (GamePartyActorHandle){0u, 0u};
}

static void game_party_emit_selection_event(
    const GameParty *party,
    const GamePartyActorState *state,
    GamePartyActorHandle handle,
    GamePartyActorHandle previous,
    uint64_t tick,
    GameEventLog *event_log,
    GamePartySelectionEventPayload *out_payload
) {
    if (out_payload) {
        out_payload->selected = handle;
        out_payload->source = state ? state->id : (GameEntityId){0u, 0u};
        out_payload->tick = tick;
        out_payload->source_faction = state ? state->faction_id : 0u;
        out_payload->previous = previous;
    }

    if (!event_log) {
        return;
    }

    GameEvent event = {
        .type = GAME_EVENT_TYPE_PARTY_SELECTED,
        .tick = tick,
        .source = state ? state->id : (GameEntityId){0u, 0u},
        .payload_size = sizeof(GamePartySelectionEventPayload),
    };
    memcpy(event.payload.bytes, & (GamePartySelectionEventPayload){
               .selected = handle,
               .previous = previous,
               .source = state ? state->id : (GameEntityId){0u, 0u},
               .tick = tick,
               .source_faction = state ? state->faction_id : 0u,
           },
           sizeof(GamePartySelectionEventPayload));
    game_event_log_append(event_log, event.tick, event.type, event.source, GAME_EVENT_LOG_INVALID_PARENT_ID, NULL);
}

GamePartyResult game_party_select_actor(
    GameParty *party,
    GamePartyActorHandle handle,
    uint64_t tick,
    GameEventLog *event_log,
    GamePartySelectionEventPayload *out_payload
) {
    if (!party || !party->slots || handle.slot >= (uint32_t)party->capacity) {
        return GAME_PARTY_RESULT_INVALID_ARGUMENT;
    }

    GamePartySlot *slot = &party->slots[handle.slot];
    if (!game_party_handle_matches(slot, handle) || !slot->alive) {
        return GAME_PARTY_RESULT_STALE_HANDLE;
    }
    if (!slot->state.selectable) {
        return GAME_PARTY_RESULT_NOT_SELECTABLE;
    }

    GamePartyActorHandle previous = party->selected_actor;
    party->selected_actor = handle;
    party->has_selection = true;
    party->last_selection_tick = tick;

    if (out_payload) {
        out_payload->selected = handle;
        out_payload->previous = previous;
        out_payload->source = slot->state.id;
        out_payload->tick = tick;
        out_payload->source_faction = slot->state.faction_id;
    }
    game_party_emit_selection_event(party, &slot->state, handle, previous, tick, event_log, NULL);
    return GAME_PARTY_RESULT_OK;
}

GamePartyResult game_party_next_selectable(
    GameParty *party,
    uint64_t tick,
    GameEventLog *event_log,
    GamePartyActorHandle *out_handle
) {
    if (!party || !out_handle || !party->slots || party->capacity == 0u) {
        return GAME_PARTY_RESULT_INVALID_ARGUMENT;
    }

    size_t start_index = 0u;
    if (party->has_selection) {
        start_index = (size_t)party->selected_actor.slot + 1u;
        if (start_index >= party->capacity) {
            start_index = 0u;
        }
    }

    for (size_t search = 0u; search < party->capacity; ++search) {
        size_t index = (start_index + search) % party->capacity;

        const GamePartySlot *slot = &party->slots[index];
        if (!slot->in_use || !slot->alive || !slot->state.selectable) {
            continue;
        }

        GamePartySelectionEventPayload payload = {0};
        GamePartyResult result = game_party_select_actor(
            party,
            (GamePartyActorHandle){.slot = (uint32_t)index, .generation = slot->generation},
            tick,
            event_log,
            &payload
        );
        if (result == GAME_PARTY_RESULT_OK) {
            *out_handle = payload.selected;
            return GAME_PARTY_RESULT_OK;
        }
    }

    return GAME_PARTY_RESULT_INVALID_HANDLE;
}

GamePartyResult game_party_clear_selection(
    GameParty *party,
    uint64_t tick,
    GameEventLog *event_log
) {
    if (!party) {
        return GAME_PARTY_RESULT_INVALID_ARGUMENT;
    }

    if (party->has_selection) {
        if (event_log) {
            GamePartyActorState state = {0};
            GamePartySelectionEventPayload payload = {
                .selected = (GamePartyActorHandle){0u, 0u},
                .previous = party->selected_actor,
                .source = (GameEntityId){0u, 0u},
                .tick = tick,
                .source_faction = 0u,
            };
            if (party->selected_actor.slot < party->capacity
                && party->slots != NULL
                && party->slots[party->selected_actor.slot].in_use) {
                payload.source = party->slots[party->selected_actor.slot].state.id;
            }
            GameEvent event = {
                .type = GAME_EVENT_TYPE_PARTY_SELECTED,
                .tick = tick,
                .source = payload.source,
                .payload_size = sizeof(GamePartySelectionEventPayload),
            };
            memcpy(event.payload.bytes, &payload, sizeof(payload));
            (void)game_event_log_append(event_log, event.tick, event.type, event.source, GAME_EVENT_LOG_INVALID_PARENT_ID, NULL);
        }
    }

    game_party_clear_slot_selection(party);
    return GAME_PARTY_RESULT_OK;
}

size_t game_party_capacity(const GameParty *party) {
    return party ? party->capacity : 0u;
}

size_t game_party_count(const GameParty *party) {
    if (!party || !party->slots || party->capacity == 0u) {
        return 0u;
    }

    size_t count = 0u;
    for (size_t i = 0u; i < party->capacity; ++i) {
        if (party->slots[i].in_use && party->slots[i].alive) {
            ++count;
        }
    }
    return count;
}

bool game_party_has_selection(const GameParty *party) {
    return party ? party->has_selection : false;
}

GamePartyActorHandle game_party_selected_handle(const GameParty *party) {
    if (!party) {
        return (GamePartyActorHandle){0u, 0u};
    }
    return party->selected_actor;
}
