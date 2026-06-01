#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "world/tile_field.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t GameFieldId;

#define GAME_FIELD_ID_NOISE ((GameFieldId)0u)
#define GAME_FIELD_ID_LIGHT ((GameFieldId)1u)
#define GAME_FIELD_ID_SCENT ((GameFieldId)2u)
#define GAME_FIELD_ID_BLOOD ((GameFieldId)3u)
#define GAME_FIELD_ID_CUSTOM_BASE ((GameFieldId)256u)
#define GAME_FIELD_ID_INVALID ((GameFieldId)0xFFFFFFFFu)

typedef enum {
    GAME_FIELD_REGISTRY_RESULT_OK = 0,
    GAME_FIELD_REGISTRY_RESULT_INVALID_ARGUMENT = 1,
    GAME_FIELD_REGISTRY_RESULT_NOT_FOUND = 2,
    GAME_FIELD_REGISTRY_RESULT_FULL = 3,
    GAME_FIELD_REGISTRY_RESULT_DISABLED = 4,
} GameFieldRegistryResult;

typedef struct {
    GameFieldId id;
    bool in_use;
    bool enabled;
    GameTileField field;
} GameFieldRegistrySlot;

typedef struct {
    GameFieldRegistrySlot *slots;
    size_t capacity;
    size_t count;
} GameFieldRegistry;

GameFieldRegistryResult game_field_registry_init(GameFieldRegistry *registry, GameFieldRegistrySlot *slots, size_t capacity);
void game_field_registry_destroy(GameFieldRegistry *registry);
GameFieldRegistryResult game_field_registry_define_field(
    GameFieldRegistry *registry,
    GameFieldId id,
    const GameTileFieldConfig *field_config
);
GameFieldRegistryResult game_field_registry_enable(GameFieldRegistry *registry, GameFieldId id, bool enabled);
GameFieldRegistryResult game_field_registry_get(
    GameFieldRegistry *registry,
    GameFieldId id,
    GameTileField **out_field
);
GameFieldRegistryResult game_field_registry_get_const(
    const GameFieldRegistry *registry,
    GameFieldId id,
    const GameTileField **out_field
);
GameFieldRegistryResult game_field_registry_collect_ids(
    const GameFieldRegistry *registry,
    GameFieldId *out_ids,
    size_t capacity,
    size_t *out_count
);

#ifdef __cplusplus
}
#endif
