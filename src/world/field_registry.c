#include "world/field_registry.h"

#include <stdint.h>
#include <string.h>

static bool game_field_registry_is_valid_slot(const GameFieldRegistry *registry) {
    return registry && registry->slots != NULL && registry->capacity > 0u;
}

static GameFieldRegistrySlot *game_field_registry_find(GameFieldRegistry *registry, GameFieldId id) {
    if (!registry) {
        return NULL;
    }
    for (size_t i = 0u; i < registry->count; ++i) {
        GameFieldRegistrySlot *slot = &registry->slots[i];
        if (slot->in_use && slot->id == id) {
            return slot;
        }
    }
    return NULL;
}

static const GameFieldRegistrySlot *game_field_registry_find_const(const GameFieldRegistry *registry, GameFieldId id) {
    if (!registry) {
        return NULL;
    }
    for (size_t i = 0u; i < registry->count; ++i) {
        const GameFieldRegistrySlot *slot = &registry->slots[i];
        if (slot->in_use && slot->id == id) {
            return slot;
        }
    }
    return NULL;
}

static void game_field_registry_cleanup_slot(GameFieldRegistrySlot *slot) {
    if (!slot) {
        return;
    }
    game_tile_field_destroy(&slot->field);
    slot->id = GAME_FIELD_ID_INVALID;
    slot->in_use = false;
    slot->enabled = false;
}

GameFieldRegistryResult game_field_registry_init(GameFieldRegistry *registry, GameFieldRegistrySlot *slots, size_t capacity) {
    if (!registry || (capacity > 0u && !slots)) {
        return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
    }

    registry->slots = slots;
    registry->capacity = capacity;
    registry->count = 0u;
    if (slots != NULL) {
        for (size_t i = 0u; i < capacity; ++i) {
            game_field_registry_cleanup_slot(&slots[i]);
        }
    }
    return GAME_FIELD_REGISTRY_RESULT_OK;
}

void game_field_registry_destroy(GameFieldRegistry *registry) {
    if (!registry || !registry->slots) {
        return;
    }
    for (size_t i = 0u; i < registry->count; ++i) {
        game_field_registry_cleanup_slot(&registry->slots[i]);
    }
    registry->slots = NULL;
    registry->capacity = 0u;
    registry->count = 0u;
}

GameFieldRegistryResult game_field_registry_define_field(
    GameFieldRegistry *registry,
    GameFieldId id,
    const GameTileFieldConfig *field_config
) {
    if (!game_field_registry_is_valid_slot(registry) || field_config == NULL) {
        return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
    }

    GameFieldRegistrySlot *slot = game_field_registry_find(registry, id);
    if (slot) {
        game_tile_field_destroy(&slot->field);
        if (game_tile_field_init(&slot->field, field_config) != GAME_TILE_FIELD_RESULT_OK) {
            return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
        }
        slot->enabled = true;
        return GAME_FIELD_REGISTRY_RESULT_OK;
    }

    if (registry->count >= registry->capacity) {
        return GAME_FIELD_REGISTRY_RESULT_FULL;
    }

    slot = &registry->slots[registry->count++];
    slot->id = id;
    slot->enabled = true;
    slot->in_use = true;
    if (game_tile_field_init(&slot->field, field_config) != GAME_TILE_FIELD_RESULT_OK) {
        slot->in_use = false;
        --registry->count;
        return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
    }
    return GAME_FIELD_REGISTRY_RESULT_OK;
}

GameFieldRegistryResult game_field_registry_enable(GameFieldRegistry *registry, GameFieldId id, bool enabled) {
    if (!game_field_registry_is_valid_slot(registry)) {
        return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
    }
    GameFieldRegistrySlot *slot = game_field_registry_find(registry, id);
    if (!slot) {
        return GAME_FIELD_REGISTRY_RESULT_NOT_FOUND;
    }
    slot->enabled = enabled;
    return GAME_FIELD_REGISTRY_RESULT_OK;
}

GameFieldRegistryResult game_field_registry_get(GameFieldRegistry *registry, GameFieldId id, GameTileField **out_field) {
    if (!registry || !out_field) {
        return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
    }
    const GameFieldRegistrySlot *slot = game_field_registry_find_const(registry, id);
    if (!slot || !slot->in_use) {
        return GAME_FIELD_REGISTRY_RESULT_NOT_FOUND;
    }
    if (!slot->enabled) {
        return GAME_FIELD_REGISTRY_RESULT_DISABLED;
    }
    *out_field = (GameTileField *)&slot->field;
    return GAME_FIELD_REGISTRY_RESULT_OK;
}

GameFieldRegistryResult game_field_registry_get_const(
    const GameFieldRegistry *registry,
    GameFieldId id,
    const GameTileField **out_field
) {
    if (!registry || !out_field) {
        return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
    }
    const GameFieldRegistrySlot *slot = game_field_registry_find_const(registry, id);
    if (!slot || !slot->in_use) {
        return GAME_FIELD_REGISTRY_RESULT_NOT_FOUND;
    }
    if (!slot->enabled) {
        return GAME_FIELD_REGISTRY_RESULT_DISABLED;
    }
    *out_field = &slot->field;
    return GAME_FIELD_REGISTRY_RESULT_OK;
}

GameFieldRegistryResult game_field_registry_collect_ids(
    const GameFieldRegistry *registry,
    GameFieldId *out_ids,
    size_t capacity,
    size_t *out_count
) {
    if (!registry || !out_count) {
        return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
    }
    if (!out_ids && capacity > 0u) {
        return GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT;
    }

    size_t enabled_count = 0u;
    for (size_t i = 0u; i < registry->count; ++i) {
        const GameFieldRegistrySlot *slot = &registry->slots[i];
        if (slot->in_use && slot->enabled) {
            ++enabled_count;
        }
    }

    if (capacity < enabled_count) {
        return GAME_FIELD_REGISTRY_RESULT_FULL;
    }

    size_t wrote = 0u;
    for (size_t i = 0u; i < registry->count; ++i) {
        const GameFieldRegistrySlot *slot = &registry->slots[i];
        if (slot->in_use && slot->enabled) {
            out_ids[wrote++] = slot->id;
        }
    }
    *out_count = wrote;
    return GAME_FIELD_REGISTRY_RESULT_OK;
}
