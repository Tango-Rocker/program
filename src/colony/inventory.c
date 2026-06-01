#include "colony/inventory.h"

#include <string.h>

static bool game_inventory_owner_match(GameInventoryOwner lhs, GameInventoryOwner rhs) {
    if (lhs.kind != rhs.kind) {
        return false;
    }

    if (lhs.kind == GAME_INVENTORY_OWNER_KIND_ENTITY) {
        return lhs.entity.index == rhs.entity.index && lhs.entity.generation == rhs.entity.generation;
    }

    return lhs.stockpile_id == rhs.stockpile_id;
}

bool game_inventory_owner_is_valid(GameInventoryOwner owner) {
    if (owner.kind == GAME_INVENTORY_OWNER_KIND_ENTITY) {
        return game_entity_id_is_valid(owner.entity);
    }

    return owner.kind == GAME_INVENTORY_OWNER_KIND_STOCKPILE && owner.stockpile_id != 0u;
}

bool game_inventory_owner_equal(GameInventoryOwner lhs, GameInventoryOwner rhs) {
    return game_inventory_owner_match(lhs, rhs);
}

static bool game_inventory_entry_handle_matches(const GameInventoryEntry *entry, GameInventoryEntryHandle handle) {
    return entry->in_use && entry->handle.slot == handle.slot && entry->handle.version == handle.version;
}

static bool game_inventory_reservation_handle_matches(
    const GameInventoryReservation *reservation,
    GameInventoryReservationHandle handle
) {
    return reservation->in_use && reservation->handle.slot == handle.slot && reservation->handle.version == handle.version;
}

static GameInventoryEntry *game_inventory_find_entry(
    GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id
) {
    if (!inventory || !inventory->entries) {
        return NULL;
    }

    for (size_t i = 0u; i < inventory->entry_capacity; ++i) {
        GameInventoryEntry *entry = &inventory->entries[i];
        if (!entry->in_use) {
            continue;
        }
        if (entry->resource_id == resource_id && game_inventory_owner_match(entry->owner, owner)) {
            return entry;
        }
    }

    return NULL;
}

static const GameInventoryEntry *game_inventory_find_entry_const(
    const GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id
) {
    if (!inventory || !inventory->entries) {
        return NULL;
    }

    for (size_t i = 0u; i < inventory->entry_capacity; ++i) {
        const GameInventoryEntry *entry = &inventory->entries[i];
        if (!entry->in_use) {
            continue;
        }
        if (entry->resource_id == resource_id && game_inventory_owner_match(entry->owner, owner)) {
            return entry;
        }
    }

    return NULL;
}

static GameInventoryEntry *game_inventory_alloc_entry(
    GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id
) {
    if (!inventory || inventory->entry_capacity == 0u || !inventory->entries) {
        return NULL;
    }

    for (size_t i = 0u; i < inventory->entry_capacity; ++i) {
        GameInventoryEntry *entry = &inventory->entries[i];
        if (entry->in_use) {
            continue;
        }
        entry->in_use = true;
        entry->owner = owner;
        entry->resource_id = resource_id;
        entry->quantity = 0u;
        entry->reserved_quantity = 0u;
        entry->handle.slot = (uint32_t)i;
        if (entry->handle.version == 0u) {
            entry->handle.version = 1u;
        }
        return entry;
    }

    return NULL;
}

static GameInventoryReservation *game_inventory_alloc_reservation(GameInventory *inventory) {
    if (!inventory || inventory->reservation_capacity == 0u || !inventory->reservations) {
        return NULL;
    }

    for (size_t i = 0u; i < inventory->reservation_capacity; ++i) {
        GameInventoryReservation *reservation = &inventory->reservations[i];
        if (reservation->in_use) {
            continue;
        }
        reservation->in_use = true;
        reservation->version += 1u;
        reservation->handle.slot = (uint32_t)i;
        if (reservation->handle.version == 0u) {
            reservation->handle.version = 1u;
        } else {
            reservation->handle.version = reservation->version;
        }
        reservation->reserved_amount = 0u;
        return reservation;
    }
    return NULL;
}

static void game_inventory_release_entry(GameInventory *inventory, GameInventoryEntry *entry) {
    if (!inventory || !entry || !entry->in_use) {
        return;
    }

    entry->in_use = false;
    entry->owner = (GameInventoryOwner){0};
    entry->resource_id = 0u;
    entry->quantity = 0u;
    entry->reserved_quantity = 0u;
    ++entry->handle.version;
}

GameInventoryResult game_inventory_init(
    GameInventory *inventory,
    GameInventoryEntry *entries,
    size_t entry_capacity,
    GameInventoryReservation *reservations,
    size_t reservation_capacity,
    uint32_t max_stack
) {
    if (!inventory || !entries || !reservations || entry_capacity == 0u || reservation_capacity == 0u || max_stack == 0u) {
        return GAME_INVENTORY_RESULT_INVALID_ARGUMENT;
    }

    inventory->entry_capacity = entry_capacity;
    inventory->reservation_capacity = reservation_capacity;
    inventory->max_stack = max_stack;
    inventory->entries = entries;
    inventory->reservations = reservations;

    game_inventory_clear(inventory);
    return GAME_INVENTORY_RESULT_OK;
}

void game_inventory_clear(GameInventory *inventory) {
    if (!inventory || !inventory->entries || !inventory->reservations) {
        if (inventory) {
            if (inventory->entries && inventory->entry_capacity == 0u && inventory->reservations && inventory->reservation_capacity == 0u) {
                memset(inventory, 0, sizeof(*inventory));
            }
        }
        return;
    }

    for (size_t i = 0u; i < inventory->entry_capacity; ++i) {
        inventory->entries[i] = (GameInventoryEntry){.handle = {(uint32_t)i, 1u}};
    }

    for (size_t i = 0u; i < inventory->reservation_capacity; ++i) {
        inventory->reservations[i] = (GameInventoryReservation){.version = 0u, .handle = {(uint32_t)i, 1u}};
    }
}

size_t game_inventory_entry_count(const GameInventory *inventory) {
    if (!inventory || !inventory->entries) {
        return 0u;
    }

    size_t count = 0u;
    for (size_t i = 0u; i < inventory->entry_capacity; ++i) {
        if (inventory->entries[i].in_use) {
            ++count;
        }
    }
    return count;
}

GameInventoryResult game_inventory_entry_status(
    const GameInventory *inventory,
    GameInventoryEntryHandle handle,
    GameInventoryEntry *out_entry
) {
    if (!inventory || !out_entry || !inventory->entries || handle.slot >= inventory->entry_capacity) {
        return GAME_INVENTORY_RESULT_INVALID_ARGUMENT;
    }

    const GameInventoryEntry *entry = &inventory->entries[handle.slot];
    if (!game_inventory_entry_handle_matches(entry, handle) || !entry->in_use) {
        return GAME_INVENTORY_RESULT_INVALID_HANDLE;
    }

    *out_entry = *entry;
    return GAME_INVENTORY_RESULT_OK;
}

size_t game_inventory_dump_all(const GameInventory *inventory, GameInventoryEntryView *out_entries, size_t out_capacity) {
    if (!inventory || !out_entries || out_capacity == 0u) {
        return 0u;
    }

    size_t out_count = 0u;
    for (size_t i = 0u; i < inventory->entry_capacity && out_count < out_capacity; ++i) {
        const GameInventoryEntry *entry = &inventory->entries[i];
        if (!entry->in_use) {
            continue;
        }
        out_entries[out_count++] = (GameInventoryEntryView){
            .handle = entry->handle,
            .owner = entry->owner,
            .resource_id = entry->resource_id,
            .total_quantity = entry->quantity,
            .reserved_quantity = entry->reserved_quantity,
        };
    }
    return out_count;
}

size_t game_inventory_dump_owner(
    const GameInventory *inventory,
    GameInventoryOwner owner,
    GameInventoryEntryView *out_entries,
    size_t out_capacity
) {
    if (!inventory || !out_entries || out_capacity == 0u || !game_inventory_owner_is_valid(owner)) {
        return 0u;
    }

    size_t out_count = 0u;
    for (size_t i = 0u; i < inventory->entry_capacity && out_count < out_capacity; ++i) {
        const GameInventoryEntry *entry = &inventory->entries[i];
        if (!entry->in_use || !game_inventory_owner_match(entry->owner, owner)) {
            continue;
        }
        out_entries[out_count++] = (GameInventoryEntryView){
            .handle = entry->handle,
            .owner = entry->owner,
            .resource_id = entry->resource_id,
            .total_quantity = entry->quantity,
            .reserved_quantity = entry->reserved_quantity,
        };
    }
    return out_count;
}

GameInventoryResult game_inventory_add(
    GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id,
    uint32_t amount,
    GameInventoryEntryHandle *out_handle
) {
    if (!inventory || !inventory->entries || amount == 0u || !game_inventory_owner_is_valid(owner)) {
        return GAME_INVENTORY_RESULT_INVALID_ARGUMENT;
    }

    GameInventoryEntry *entry = game_inventory_find_entry(inventory, owner, resource_id);
    if (!entry) {
        entry = game_inventory_alloc_entry(inventory, owner, resource_id);
        if (!entry) {
            return GAME_INVENTORY_RESULT_OUT_OF_SPACE;
        }
    }

    if (UINT32_MAX - entry->quantity < amount || entry->quantity + amount > inventory->max_stack) {
        return GAME_INVENTORY_RESULT_OUT_OF_SPACE;
    }

    entry->quantity += amount;
    if (out_handle) {
        *out_handle = entry->handle;
    }
    return GAME_INVENTORY_RESULT_OK;
}

GameInventoryResult game_inventory_remove(
    GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id,
    uint32_t amount,
    uint32_t *out_removed
) {
    if (!inventory || !inventory->entries || amount == 0u || !game_inventory_owner_is_valid(owner)) {
        return GAME_INVENTORY_RESULT_INVALID_ARGUMENT;
    }

    GameInventoryEntry *entry = game_inventory_find_entry(inventory, owner, resource_id);
    if (!entry) {
        return GAME_INVENTORY_RESULT_ENTRY_NOT_FOUND;
    }

    if (entry->reserved_quantity > 0u) {
        if (out_removed) {
            *out_removed = 0u;
        }
        return GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY;
    }

    uint32_t available = entry->quantity - entry->reserved_quantity;
    if (available < amount) {
        if (out_removed) {
            *out_removed = 0u;
        }
        return GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY;
    }

    entry->quantity -= amount;
    if (out_removed) {
        *out_removed = amount;
    }

    if (entry->quantity == 0u && entry->reserved_quantity == 0u) {
        game_inventory_release_entry(inventory, entry);
    }

    return GAME_INVENTORY_RESULT_OK;
}

GameInventoryResult game_inventory_reserve(
    GameInventory *inventory,
    GameInventoryOwner owner,
    uint32_t resource_id,
    uint32_t amount,
    GameInventoryReservationHandle *out_reservation
) {
    if (!inventory || !inventory->reservations || !game_inventory_owner_is_valid(owner) || amount == 0u) {
        return GAME_INVENTORY_RESULT_INVALID_ARGUMENT;
    }

    GameInventoryEntry *entry = game_inventory_find_entry(inventory, owner, resource_id);
    if (!entry) {
        return GAME_INVENTORY_RESULT_ENTRY_NOT_FOUND;
    }

    uint32_t available = entry->quantity - entry->reserved_quantity;
    if (available < amount) {
        return GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY;
    }

    GameInventoryReservation *reservation = game_inventory_alloc_reservation(inventory);
    if (!reservation) {
        return GAME_INVENTORY_RESULT_RESERVATION_FULL;
    }

    reservation->entry = entry->handle;
    reservation->reserved_amount = amount;
    entry->reserved_quantity += amount;
    if (out_reservation) {
        *out_reservation = reservation->handle;
    }
    return GAME_INVENTORY_RESULT_OK;
}

GameInventoryResult game_inventory_release_reservation(GameInventory *inventory, GameInventoryReservationHandle reservation) {
    if (!inventory || !inventory->reservations || reservation.slot >= inventory->reservation_capacity) {
        return GAME_INVENTORY_RESULT_INVALID_ARGUMENT;
    }

    GameInventoryReservation *slot = &inventory->reservations[reservation.slot];
    if (!game_inventory_reservation_handle_matches(slot, reservation) || !slot->in_use) {
        return GAME_INVENTORY_RESULT_INVALID_HANDLE;
    }

    if (slot->entry.slot >= inventory->entry_capacity) {
        return GAME_INVENTORY_RESULT_RESERVATION_STALE;
    }

    GameInventoryEntry *entry = &inventory->entries[slot->entry.slot];
    if (!entry->in_use) {
        slot->in_use = false;
        return GAME_INVENTORY_RESULT_RESERVATION_STALE;
    }

    if (entry->reserved_quantity < slot->reserved_amount) {
        entry->reserved_quantity = 0u;
    } else {
        entry->reserved_quantity -= slot->reserved_amount;
    }

    slot->in_use = false;
    slot->reserved_amount = 0u;
    slot->entry = (GameInventoryEntryHandle){0u, 0u};
    return GAME_INVENTORY_RESULT_OK;
}

GameInventoryResult game_inventory_commit_reservation(
    GameInventory *inventory,
    GameInventoryReservationHandle reservation,
    uint32_t *out_committed_amount
) {
    if (!inventory || !inventory->reservations || reservation.slot >= inventory->reservation_capacity) {
        return GAME_INVENTORY_RESULT_INVALID_ARGUMENT;
    }

    GameInventoryReservation *slot = &inventory->reservations[reservation.slot];
    if (!game_inventory_reservation_handle_matches(slot, reservation) || !slot->in_use) {
        return GAME_INVENTORY_RESULT_INVALID_HANDLE;
    }

    if (slot->entry.slot >= inventory->entry_capacity) {
        return GAME_INVENTORY_RESULT_RESERVATION_STALE;
    }

    GameInventoryEntry *entry = &inventory->entries[slot->entry.slot];
    if (!entry->in_use || entry->reserved_quantity < slot->reserved_amount) {
        return GAME_INVENTORY_RESULT_RESERVATION_STALE;
    }

    if (out_committed_amount) {
        *out_committed_amount = slot->reserved_amount;
    }

    if (entry->quantity < slot->reserved_amount) {
        return GAME_INVENTORY_RESULT_INSUFFICIENT_QUANTITY;
    }

    entry->quantity -= slot->reserved_amount;
    entry->reserved_quantity -= slot->reserved_amount;
    slot->in_use = false;
    slot->reserved_amount = 0u;
    slot->entry = (GameInventoryEntryHandle){0u, 0u};

    if (entry->quantity == 0u && entry->reserved_quantity == 0u) {
        game_inventory_release_entry(inventory, entry);
    }

    return GAME_INVENTORY_RESULT_OK;
}

GameInventoryResult game_inventory_reservation_status(
    const GameInventory *inventory,
    GameInventoryReservationHandle reservation,
    GameInventoryReservation *out_reservation
) {
    if (!inventory || !out_reservation || !inventory->reservations || reservation.slot >= inventory->reservation_capacity) {
        return GAME_INVENTORY_RESULT_INVALID_ARGUMENT;
    }

    const GameInventoryReservation *slot = &inventory->reservations[reservation.slot];
    if (!game_inventory_reservation_handle_matches(slot, reservation)) {
        return GAME_INVENTORY_RESULT_INVALID_HANDLE;
    }

    *out_reservation = *slot;
    return GAME_INVENTORY_RESULT_OK;
}

size_t game_inventory_total_reserved(const GameInventory *inventory, GameInventoryOwner owner, uint32_t resource_id) {
    if (!inventory || !game_inventory_owner_is_valid(owner)) {
        return 0u;
    }

    const GameInventoryEntry *entry = game_inventory_find_entry_const(inventory, owner, resource_id);
    return entry ? (size_t)entry->reserved_quantity : 0u;
}
