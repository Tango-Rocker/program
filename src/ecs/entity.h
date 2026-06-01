#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GameEntityId {
    uint32_t index;
    uint32_t generation;
} GameEntityId;

typedef enum {
    GAME_ENTITY_RESULT_OK = 0,
    GAME_ENTITY_RESULT_INVALID_ARGUMENT = 1,
    GAME_ENTITY_RESULT_STALE_HANDLE = 2,
    GAME_ENTITY_RESULT_POOL_FULL = 3,
} GameEntityResult;

typedef struct GameEntityRegistry {
    size_t capacity;
    uint32_t next_index;
    uint32_t free_head;
    uint32_t free_tail;
    uint32_t free_count;
    uint32_t *generations;
    bool *alive;
    uint32_t *free_indices;
} GameEntityRegistry;

GameEntityId game_entity_invalid_id(void);
bool game_entity_id_is_valid(GameEntityId id);

GameEntityResult game_entity_registry_init(GameEntityRegistry *registry, size_t capacity);
void game_entity_registry_destroy(GameEntityRegistry *registry);
GameEntityResult game_entity_registry_create(GameEntityRegistry *registry, GameEntityId *out_id);
GameEntityResult game_entity_registry_destroy_id(GameEntityRegistry *registry, GameEntityId id);
bool game_entity_registry_is_alive(const GameEntityRegistry *registry, GameEntityId id);
size_t game_entity_registry_capacity(const GameEntityRegistry *registry);

#ifdef __cplusplus
}
#endif
