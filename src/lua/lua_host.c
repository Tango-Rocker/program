#include "lua/lua_host.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static bool game_lua_host_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void game_lua_host_trim(char *line) {
    if (!line) {
        return;
    }

    size_t start = 0u;
    while (line[start] != '\0' && game_lua_host_is_space(line[start])) {
        ++start;
    }
    if (start > 0u) {
        size_t write = 0u;
        while (line[start] != '\0') {
            line[write++] = line[start++];
        }
        line[write] = '\0';
    }

    size_t end = strlen(line);
    while (end > 0u && game_lua_host_is_space(line[end - 1u])) {
        --end;
    }
    line[end] = '\0';
}

static bool game_lua_host_starts_with(const char *text, const char *prefix) {
    if (!text || !prefix) {
        return false;
    }
    while (*prefix) {
        if (*text++ != *prefix++) {
            return false;
        }
    }
    return true;
}

GameLuaHostResult game_lua_host_init_prototype_set(
    GameLuaWorkerPrototypeSet *set,
    GameLuaWorkerPrototype *entries,
    size_t capacity
) {
    if (!set || (!entries && capacity > 0u)) {
        return GAME_LUA_HOST_RESULT_INVALID_ARGUMENT;
    }

    set->entries = entries;
    set->count = 0u;
    set->capacity = capacity;
    return GAME_LUA_HOST_RESULT_OK;
}

GameLuaHostResult game_lua_host_load_worker_prototypes(const char *path, GameLuaWorkerPrototypeSet *out_set) {
    if (!path || !out_set || !out_set->entries) {
        return GAME_LUA_HOST_RESULT_INVALID_ARGUMENT;
    }

    FILE *handle = fopen(path, "r");
    if (!handle) {
        return GAME_LUA_HOST_RESULT_MISSING_FILE;
    }

    char line[256];
    out_set->count = 0u;
    bool parsed_any = false;
    GameLuaHostResult result = GAME_LUA_HOST_RESULT_OK;

    while (fgets(line, (int)sizeof(line), handle) != NULL) {
        game_lua_host_trim(line);
        if (line[0] == '\0' || game_lua_host_starts_with(line, "--")) {
            continue;
        }
        if (game_lua_host_starts_with(line, "ability ") || game_lua_host_starts_with(line, "job ")
            || game_lua_host_starts_with(line, "effect ")) {
            continue;
        }

        char role_id[sizeof(((GameLuaWorkerPrototype *)0)->role_id)] = {0};
        unsigned int stamina = 0u;
        unsigned int threat_threshold = 0u;

        if (sscanf(line, "prototype \"%31[^\"]\" %u %u", role_id, &stamina, &threat_threshold) != 3) {
            result = GAME_LUA_HOST_RESULT_INVALID_FORMAT;
            break;
        }

        if (out_set->count >= out_set->capacity) {
            result = GAME_LUA_HOST_RESULT_BUFFER_TOO_SMALL;
            break;
        }

        GameLuaWorkerPrototype *entry = &out_set->entries[out_set->count++];
        (void)snprintf(entry->role_id, sizeof(entry->role_id), "%s", role_id);
        entry->stamina = stamina;
        entry->threat_threshold = threat_threshold;
        parsed_any = true;
    }

    fclose(handle);

    if (result != GAME_LUA_HOST_RESULT_OK) {
        out_set->count = 0u;
        return result;
    }
    if (!parsed_any) {
        return GAME_LUA_HOST_RESULT_INVALID_FORMAT;
    }

    return GAME_LUA_HOST_RESULT_OK;
}

const GameLuaWorkerPrototype *game_lua_host_find_worker_prototype(
    const GameLuaWorkerPrototypeSet *set,
    const char *role_id
) {
    if (!set || !set->entries || !role_id) {
        return NULL;
    }

    for (size_t i = 0u; i < set->count; ++i) {
        const GameLuaWorkerPrototype *entry = &set->entries[i];
        if (entry && entry->role_id[0] != '\0' && strcmp(entry->role_id, role_id) == 0) {
            return entry;
        }
    }

    return NULL;
}
