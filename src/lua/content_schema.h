#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_CONTENT_SCHEMA_RESULT_OK = 0,
    GAME_CONTENT_SCHEMA_RESULT_INVALID_ARGUMENT = 1,
    GAME_CONTENT_SCHEMA_RESULT_MISSING_FILE = 2,
    GAME_CONTENT_SCHEMA_RESULT_INVALID_FORMAT = 3,
    GAME_CONTENT_SCHEMA_RESULT_DUPLICATE_ID = 4,
    GAME_CONTENT_SCHEMA_RESULT_OUT_OF_RANGE = 5,
    GAME_CONTENT_SCHEMA_RESULT_BUFFER_TOO_SMALL = 6,
    GAME_CONTENT_SCHEMA_RESULT_MISSING_REQUIRED = 7,
} GameContentSchemaResult;

typedef struct {
    char id[32];
    uint32_t damage;
    uint32_t cooldown_ticks;
} GameAbilityPrototype;

typedef struct {
    char id[32];
    uint32_t priority;
    uint32_t work_ticks;
} GameJobPrototype;

typedef struct {
    char id[32];
    uint32_t duration_ticks;
    uint32_t magnitude;
} GameStatusEffectPrototype;

typedef struct {
    GameAbilityPrototype *abilities;
    size_t ability_count;
    size_t ability_capacity;
    GameJobPrototype *jobs;
    size_t job_count;
    size_t job_capacity;
    GameStatusEffectPrototype *effects;
    size_t effect_count;
    size_t effect_capacity;
} GameContentPrototypeSet;

typedef struct {
    GameContentSchemaResult result;
    size_t line;
    char message[96];
} GameContentSchemaError;

GameContentSchemaResult game_content_schema_init_set(
    GameContentPrototypeSet *set,
    GameAbilityPrototype *abilities,
    size_t ability_capacity,
    GameJobPrototype *jobs,
    size_t job_capacity,
    GameStatusEffectPrototype *effects,
    size_t effect_capacity
);
GameContentSchemaResult game_content_schema_load(
    const char *path,
    GameContentPrototypeSet *out_set,
    GameContentSchemaError *out_error
);
const GameAbilityPrototype *game_content_schema_find_ability(const GameContentPrototypeSet *set, const char *id);
const GameJobPrototype *game_content_schema_find_job(const GameContentPrototypeSet *set, const char *id);
const GameStatusEffectPrototype *game_content_schema_find_effect(const GameContentPrototypeSet *set, const char *id);

#ifdef __cplusplus
}
#endif
