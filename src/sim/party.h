#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "world/hex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_PARTY_RESULT_OK = 0,
    GAME_PARTY_RESULT_INVALID_ARGUMENT = 1,
    GAME_PARTY_RESULT_BUFFER_TOO_SMALL = 2,
    GAME_PARTY_RESULT_INVALID_HANDLE = 3,
    GAME_PARTY_RESULT_STALE_HANDLE = 4,
    GAME_PARTY_RESULT_NOT_SELECTABLE = 5,
} GamePartyResult;

typedef struct {
    uint32_t slot;
    uint32_t generation;
} GamePartyActorHandle;

typedef struct {
    GameEntityId id;
    GameHexAxial position;
    int32_t health;
    uint32_t faction_id;
    bool selectable;
} GamePartyActorState;

typedef struct {
    GamePartyActorHandle handle;
    uint32_t generation;
    bool in_use;
    bool alive;
    GamePartyActorState state;
} GamePartySlot;

typedef struct {
    size_t capacity;
    GamePartySlot *slots;
    GamePartyActorHandle selected_actor;
    bool has_selection;
    uint64_t last_selection_tick;
} GameParty;

typedef struct {
    GamePartyActorHandle selected;
    GamePartyActorHandle previous;
    GameEntityId source;
    uint64_t tick;
    uint32_t source_faction;
} GamePartySelectionEventPayload;

void game_party_init(GameParty *party, GamePartySlot *slots, size_t capacity);
GamePartyResult game_party_register_actor(
    GameParty *party,
    const GamePartyActorState *state,
    GamePartyActorHandle *out_handle
);
GamePartyResult game_party_select_actor(
    GameParty *party,
    GamePartyActorHandle handle,
    uint64_t tick,
    GameEventLog *event_log,
    GamePartySelectionEventPayload *out_payload
);
GamePartyResult game_party_next_selectable(
    GameParty *party,
    uint64_t tick,
    GameEventLog *event_log,
    GamePartyActorHandle *out_handle
);
GamePartyResult game_party_clear_selection(
    GameParty *party,
    uint64_t tick,
    GameEventLog *event_log
);
bool game_party_is_valid_handle(const GameParty *party, GamePartyActorHandle handle);
GamePartyResult game_party_get_actor(const GameParty *party, GamePartyActorHandle handle, GamePartyActorState *out_state);
size_t game_party_capacity(const GameParty *party);
size_t game_party_count(const GameParty *party);
bool game_party_has_selection(const GameParty *party);
GamePartyActorHandle game_party_selected_handle(const GameParty *party);
GamePartyResult game_party_set_selectable(GameParty *party, GamePartyActorHandle handle, bool selectable);

#ifdef __cplusplus
}
#endif
