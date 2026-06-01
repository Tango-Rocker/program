#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ecs/entity.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_INVENTORY_RESULT_OK = 0,
    GAME_INVENTORY_RESULT_INVALID_ARGUMENT = 1,
    GAME_INVENTORY_RESULT_INVALID_OWNER = 2,
    GAME_INVENTORY_RESULT_INVALID_HANDLE = 3,
    GAME_INVENTORY_RESULT_OUT_OF_SPACE = 4,
    GAME_INVENTORY_RESULT_ENTRY_NOT_FOUND = 5,
    GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY = 6,
    GAME_INVENTORY_RESULT_RESERVATION_FULL = 7,
    GAME_INVENTORY_RESULT_RESERVATION_NOT_FOUND = 8,
    GAME_INVENTORY_RESULT_RESERVATION_STALE = 9,
} GameInventoryResult;

typedef enum {
    GAME_INVENTORY_OWNER_KIND_ENTITY = 0,
    GAME_INVENTORY_OWNER_KIND_STOCKPILE = 1,
} GameInventoryOwnerKind;

typedef struct {
    GameInventoryOwnerKind kind;
    GameEntityId entity;
    uint32_t stockpile_id;
} GameInventoryOwner;

typedef struct {
    uint32_t slot;
    uint32_t version;
} GameInventoryEntryHandle;

typedef struct {
    uint32_t slot;
    uint32_t version;
} GameInventoryReservationHandle;

typedef struct {
    bool in_use;
    GameInventoryEntryHandle handle;
    GameInventoryOwner owner;
    uint32_t resource_id;
    uint32_t quantity;
    uint32_t reserved_quantity;
} GameInventoryEntry;

typedef struct {
    bool in_use;
    uint32_t version;
    GameInventoryReservationHandle handle;
    GameInventoryEntryHandle entry;
    uint32_t reserved_amount;
} GameInventoryReservation;

typedef struct {
    size_t entry_capacity;
    size_t reservation_capacity;
    uint32_t max_stack;
    GameInventoryEntry *entries;
    GameInventoryReservation *reservations;
} GameInventory;

typedef struct {
    GameInventoryEntryHandle handle;
    GameInventoryOwner owner;
    uint32_t resource_id;
    uint32_t total_quantity;
    uint32_t reserved_quantity;
} GameInventoryEntryView;

bool game_inventory_owner_is_valid(GameInventoryOwner owner);
bool game_inventory_owner_equal(GameInventoryOwner lhs, GameInventoryOwner rhs);

GameInventoryResult game_inventory_init(
    GameInventory *inventory,
    GameInventoryEntry *entries,
    size_t entry_capacity,
    GameInventoryReservation *reservations,
    size_t reservation_capacity,
    uint32_t max_stack
);

void game_inventory_clear(GameInventory *inventory);

size_t game_inventory_entry_count(const GameInventory *inventory);
GameInventoryResult game_inventory_entry_status(
    const GameInventory *inventory,
    GameInventoryEntryHandle handle,
    GameInventoryEntry *out_entry
);

size_t game_inventory_dump_all(const GameInventory *inventory, GameInventoryEntryView *out_entries, size_t out_capacity);
size_t game_inventory_dump_owner(
    const GameInventory *inventory,
    GameInventoryOwner owner,
    GameInventoryEntryView *out_entries,
    size_t out_capacity
);

GameInventoryResult game_inventory_add(
    GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id,
    uint32_t amount,
    GameInventoryEntryHandle *out_handle
);

GameInventoryResult game_inventory_remove(
    GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id,
    uint32_t amount,
    uint32_t *out_removed
);

GameInventoryResult game_inventory_reserve(
    GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id,
    uint32_t amount,
    GameInventoryReservationHandle *out_reservation
);

GameInventoryResult game_inventory_release_reservation(
    GameInventory *inventory,
    GameInventoryReservationHandle reservation
);

GameInventoryResult game_inventory_commit_reservation(
    GameInventory *inventory,
    GameInventoryReservationHandle reservation,
    uint32_t *out_committed_amount
);

GameInventoryResult game_inventory_reservation_status(
    const GameInventory *inventory,
    GameInventoryReservationHandle reservation,
    GameInventoryReservation *out_reservation
);

size_t game_inventory_total_reserved(const GameInventory *inventory, GameInventoryOwner owner, uint32_t resource_id);

#ifdef __cplusplus
}
#endif
