#include <stdio.h>

#include "lua/lua_host.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[lua_host] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_lua_host(void) {
    int failed = 0;

    GameLuaWorkerPrototype prototypes[8] = {0};
    GameLuaWorkerPrototypeSet set = {0};
    failed += assert_true(
        game_lua_host_init_prototype_set(&set, prototypes, 8u) == GAME_LUA_HOST_RESULT_OK,
        "init prototype set"
    );
    failed += assert_true(
        game_lua_host_load_worker_prototypes("data/lua/prototypes.lua", &set)
            == GAME_LUA_HOST_RESULT_OK,
        "load valid worker fixture"
    );
    failed += assert_true(set.count == 3u, "three prototype rows loaded");
    const GameLuaWorkerPrototype *miner = game_lua_host_find_worker_prototype(&set, "miner");
    failed += assert_true(miner && miner->stamina == 100u, "miner stamina parsed");
    failed += assert_true(miner && miner->threat_threshold == 2u, "miner threat threshold parsed");

    failed += assert_true(
        game_lua_host_load_worker_prototypes("data/lua/missing.lua", &set)
            == GAME_LUA_HOST_RESULT_MISSING_FILE,
        "missing file rejected"
    );

    GameLuaWorkerPrototype invalid_set_storage[2] = {0};
    GameLuaWorkerPrototypeSet invalid_set = {
        .entries = invalid_set_storage,
        .count = 0u,
        .capacity = 2u,
    };
    failed += assert_true(
        game_lua_host_load_worker_prototypes("data/lua/prototypes_invalid.lua", &invalid_set)
            == GAME_LUA_HOST_RESULT_INVALID_FORMAT,
        "invalid fixture format rejected"
    );

    if (failed == 0) {
        printf("[lua_host] PASS\n");
    }
    return failed;
}
