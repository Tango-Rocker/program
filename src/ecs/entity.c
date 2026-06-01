#include "ecs/entity.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

GameEntityId game_entity_invalid_id(void) {
    return (GameEntityId){UINT32_MAX, UINT32_MAX};
}

bool game_entity_id_is_valid(GameEntityId id) {
    return id.index != UINT32_MAX && id.generation != UINT32_MAX;
}

size_t game_entity_registry_capacity(const GameEntityRegistry *registry) {
    return registry ? registry->capacity : 0;
}

GameEntityResult game_entity_registry_init(GameEntityRegistry *registry, size_t capacity) {
    if (!registry || capacity == 0) {
        return GAME_ENTITY_RESULT_INVALID_ARGUMENT;
    }

    memset(registry, 0, sizeof(*registry));

    if (capacity > UINT32_MAX) {
        return GAME_ENTITY_RESULT_INVALID_ARGUMENT;
    }

    registry->generations = (uint32_t *)calloc(capacity, sizeof(uint32_t));
    if (!registry->generations) {
        game_entity_registry_destroy(registry);
        return GAME_ENTITY_RESULT_INVALID_ARGUMENT;
    }

    registry->alive = (bool *)calloc(capacity, sizeof(bool));
    if (!registry->alive) {
        game_entity_registry_destroy(registry);
        return GAME_ENTITY_RESULT_INVALID_ARGUMENT;
    }

    registry->free_indices = (uint32_t *)malloc(capacity * sizeof(uint32_t));
    if (!registry->free_indices) {
        game_entity_registry_destroy(registry);
        return GAME_ENTITY_RESULT_INVALID_ARGUMENT;
    }

    registry->capacity = capacity;
    return GAME_ENTITY_RESULT_OK;
}

void game_entity_registry_destroy(GameEntityRegistry *registry) {
    if (!registry) {
        return;
    }

    free(registry->generations);
    free(registry->alive);
    free(registry->free_indices);
    memset(registry, 0, sizeof(*registry));
}

GameEntityResult game_entity_registry_create(GameEntityRegistry *registry, GameEntityId *out_id) {
    if (!registry || !out_id) {
        return GAME_ENTITY_RESULT_INVALID_ARGUMENT;
    }

    uint32_t index = UINT32_MAX;

    if (registry->free_count > 0) {
        index = registry->free_indices[registry->free_head];
        registry->free_head = (registry->free_head + 1U) % (uint32_t)registry->capacity;
        registry->free_count--;
    } else if (registry->next_index < registry->capacity) {
        index = registry->next_index++;
    } else {
        return GAME_ENTITY_RESULT_POOL_FULL;
    }

    registry->alive[index] = true;
    *out_id = (GameEntityId){index, registry->generations[index]};
    return GAME_ENTITY_RESULT_OK;
}

GameEntityResult game_entity_registry_destroy_id(GameEntityRegistry *registry, GameEntityId id) {
    if (!registry || !game_entity_id_is_valid(id)) {
        return GAME_ENTITY_RESULT_INVALID_ARGUMENT;
    }

    if (id.index >= registry->capacity) {
        return GAME_ENTITY_RESULT_STALE_HANDLE;
    }

    if (id.generation != registry->generations[id.index] || !registry->alive[id.index]) {
        return GAME_ENTITY_RESULT_STALE_HANDLE;
    }

    registry->alive[id.index] = false;
    registry->generations[id.index]++;
    registry->free_indices[registry->free_tail] = id.index;
    registry->free_tail = (registry->free_tail + 1U) % (uint32_t)registry->capacity;
    registry->free_count++;

    return GAME_ENTITY_RESULT_OK;
}

bool game_entity_registry_is_alive(const GameEntityRegistry *registry, GameEntityId id) {
    if (!registry || !game_entity_id_is_valid(id)) {
        return false;
    }

    if (id.index >= registry->capacity) {
        return false;
    }

    if (id.generation != registry->generations[id.index]) {
        return false;
    }

    return registry->alive[id.index];
}
