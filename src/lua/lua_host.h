#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GAME_LUA_HOST_RESULT_OK = 0,
    GAME_LUA_HOST_RESULT_INVALID_ARGUMENT = 1,
    GAME_LUA_HOST_RESULT_MISSING_FILE = 2,
    GAME_LUA_HOST_RESULT_INVALID_FORMAT = 3,
    GAME_LUA_HOST_RESULT_BUFFER_TOO_SMALL = 4,
} GameLuaHostResult;

typedef struct {
    char role_id[32];
    uint32_t stamina;
    uint32_t threat_threshold;
} GameLuaWorkerPrototype;

typedef struct {
    GameLuaWorkerPrototype *entries;
    size_t count;
    size_t capacity;
} GameLuaWorkerPrototypeSet;

GameLuaHostResult game_lua_host_init_prototype_set(
    GameLuaWorkerPrototypeSet *set,
    GameLuaWorkerPrototype *entries,
    size_t capacity
);

GameLuaHostResult game_lua_host_load_worker_prototypes(const char *path, GameLuaWorkerPrototypeSet *out_set);
const GameLuaWorkerPrototype *game_lua_host_find_worker_prototype(
    const GameLuaWorkerPrototypeSet *set,
    const char *role_id
);

#ifdef __cplusplus
}
#endif
