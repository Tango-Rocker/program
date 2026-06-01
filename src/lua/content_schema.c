#include "lua/content_schema.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void game_content_schema_error(
    GameContentSchemaError *out_error,
    GameContentSchemaResult result,
    size_t line,
    const char *message
) {
    if (!out_error) {
        return;
    }
    out_error->result = result;
    out_error->line = line;
    (void)snprintf(out_error->message, sizeof(out_error->message), "%s", message ? message : "");
}

static bool game_content_schema_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void game_content_schema_trim(char *line) {
    size_t start = 0u;
    while (line[start] != '\0' && game_content_schema_is_space(line[start])) {
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
    while (end > 0u && game_content_schema_is_space(line[end - 1u])) {
        --end;
    }
    line[end] = '\0';
}

static bool game_content_schema_duplicate_ability(const GameContentPrototypeSet *set, const char *id) {
    return game_content_schema_find_ability(set, id) != NULL;
}

static bool game_content_schema_duplicate_job(const GameContentPrototypeSet *set, const char *id) {
    return game_content_schema_find_job(set, id) != NULL;
}

static bool game_content_schema_duplicate_effect(const GameContentPrototypeSet *set, const char *id) {
    return game_content_schema_find_effect(set, id) != NULL;
}

GameContentSchemaResult game_content_schema_init_set(
    GameContentPrototypeSet *set,
    GameAbilityPrototype *abilities,
    size_t ability_capacity,
    GameJobPrototype *jobs,
    size_t job_capacity,
    GameStatusEffectPrototype *effects,
    size_t effect_capacity
) {
    if (!set || (!abilities && ability_capacity > 0u) || (!jobs && job_capacity > 0u)
        || (!effects && effect_capacity > 0u)) {
        return GAME_CONTENT_SCHEMA_RESULT_INVALID_ARGUMENT;
    }
    *set = (GameContentPrototypeSet){
        .abilities = abilities,
        .ability_capacity = ability_capacity,
        .jobs = jobs,
        .job_capacity = job_capacity,
        .effects = effects,
        .effect_capacity = effect_capacity,
    };
    return GAME_CONTENT_SCHEMA_RESULT_OK;
}

GameContentSchemaResult game_content_schema_load(
    const char *path,
    GameContentPrototypeSet *out_set,
    GameContentSchemaError *out_error
) {
    if (!path || !out_set || !out_set->abilities || !out_set->jobs || !out_set->effects) {
        game_content_schema_error(out_error, GAME_CONTENT_SCHEMA_RESULT_INVALID_ARGUMENT, 0u, "invalid arguments");
        return GAME_CONTENT_SCHEMA_RESULT_INVALID_ARGUMENT;
    }

    FILE *handle = fopen(path, "r");
    if (!handle) {
        game_content_schema_error(out_error, GAME_CONTENT_SCHEMA_RESULT_MISSING_FILE, 0u, "missing content file");
        return GAME_CONTENT_SCHEMA_RESULT_MISSING_FILE;
    }

    GameContentPrototypeSet next = *out_set;
    next.ability_count = 0u;
    next.job_count = 0u;
    next.effect_count = 0u;

    char line[256];
    size_t line_number = 0u;
    GameContentSchemaResult result = GAME_CONTENT_SCHEMA_RESULT_OK;
    while (fgets(line, (int)sizeof(line), handle) != NULL) {
        ++line_number;
        game_content_schema_trim(line);
        if (line[0] == '\0' || (line[0] == '-' && line[1] == '-')) {
            continue;
        }
        if (strncmp(line, "prototype ", 10u) == 0) {
            continue;
        }

        char id[32] = {0};
        unsigned int first = 0u;
        unsigned int second = 0u;
        if (sscanf(line, "ability \"%31[^\"]\" %u %u", id, &first, &second) == 3) {
            if (id[0] == '\0' || first == 0u || first > 10000u || second > 100000u) {
                result = GAME_CONTENT_SCHEMA_RESULT_OUT_OF_RANGE;
                game_content_schema_error(out_error, result, line_number, "ability value out of range");
                break;
            }
            if (game_content_schema_duplicate_ability(&next, id)) {
                result = GAME_CONTENT_SCHEMA_RESULT_DUPLICATE_ID;
                game_content_schema_error(out_error, result, line_number, "duplicate ability id");
                break;
            }
            if (next.ability_count >= next.ability_capacity) {
                result = GAME_CONTENT_SCHEMA_RESULT_BUFFER_TOO_SMALL;
                game_content_schema_error(out_error, result, line_number, "ability buffer too small");
                break;
            }
            GameAbilityPrototype *entry = &next.abilities[next.ability_count++];
            (void)snprintf(entry->id, sizeof(entry->id), "%s", id);
            entry->damage = first;
            entry->cooldown_ticks = second;
            continue;
        }
        if (sscanf(line, "job \"%31[^\"]\" %u %u", id, &first, &second) == 3) {
            if (id[0] == '\0' || first == 0u || first > 100u || second == 0u || second > 100000u) {
                result = GAME_CONTENT_SCHEMA_RESULT_OUT_OF_RANGE;
                game_content_schema_error(out_error, result, line_number, "job value out of range");
                break;
            }
            if (game_content_schema_duplicate_job(&next, id)) {
                result = GAME_CONTENT_SCHEMA_RESULT_DUPLICATE_ID;
                game_content_schema_error(out_error, result, line_number, "duplicate job id");
                break;
            }
            if (next.job_count >= next.job_capacity) {
                result = GAME_CONTENT_SCHEMA_RESULT_BUFFER_TOO_SMALL;
                game_content_schema_error(out_error, result, line_number, "job buffer too small");
                break;
            }
            GameJobPrototype *entry = &next.jobs[next.job_count++];
            (void)snprintf(entry->id, sizeof(entry->id), "%s", id);
            entry->priority = first;
            entry->work_ticks = second;
            continue;
        }
        if (sscanf(line, "effect \"%31[^\"]\" %u %u", id, &first, &second) == 3) {
            if (id[0] == '\0' || first == 0u || first > 100000u || second == 0u || second > 10000u) {
                result = GAME_CONTENT_SCHEMA_RESULT_OUT_OF_RANGE;
                game_content_schema_error(out_error, result, line_number, "effect value out of range");
                break;
            }
            if (game_content_schema_duplicate_effect(&next, id)) {
                result = GAME_CONTENT_SCHEMA_RESULT_DUPLICATE_ID;
                game_content_schema_error(out_error, result, line_number, "duplicate effect id");
                break;
            }
            if (next.effect_count >= next.effect_capacity) {
                result = GAME_CONTENT_SCHEMA_RESULT_BUFFER_TOO_SMALL;
                game_content_schema_error(out_error, result, line_number, "effect buffer too small");
                break;
            }
            GameStatusEffectPrototype *entry = &next.effects[next.effect_count++];
            (void)snprintf(entry->id, sizeof(entry->id), "%s", id);
            entry->duration_ticks = first;
            entry->magnitude = second;
            continue;
        }

        result = GAME_CONTENT_SCHEMA_RESULT_INVALID_FORMAT;
        game_content_schema_error(out_error, result, line_number, "unrecognized prototype row");
        break;
    }
    fclose(handle);

    if (result == GAME_CONTENT_SCHEMA_RESULT_OK
        && (next.ability_count == 0u || next.job_count == 0u || next.effect_count == 0u)) {
        result = GAME_CONTENT_SCHEMA_RESULT_MISSING_REQUIRED;
        game_content_schema_error(out_error, result, line_number, "missing required prototype type");
    }

    if (result != GAME_CONTENT_SCHEMA_RESULT_OK) {
        return result;
    }

    out_set->ability_count = next.ability_count;
    out_set->job_count = next.job_count;
    out_set->effect_count = next.effect_count;
    game_content_schema_error(out_error, GAME_CONTENT_SCHEMA_RESULT_OK, 0u, "ok");
    return GAME_CONTENT_SCHEMA_RESULT_OK;
}

const GameAbilityPrototype *game_content_schema_find_ability(const GameContentPrototypeSet *set, const char *id) {
    if (!set || !id) {
        return NULL;
    }
    for (size_t i = 0u; i < set->ability_count; ++i) {
        if (strcmp(set->abilities[i].id, id) == 0) {
            return &set->abilities[i];
        }
    }
    return NULL;
}

const GameJobPrototype *game_content_schema_find_job(const GameContentPrototypeSet *set, const char *id) {
    if (!set || !id) {
        return NULL;
    }
    for (size_t i = 0u; i < set->job_count; ++i) {
        if (strcmp(set->jobs[i].id, id) == 0) {
            return &set->jobs[i];
        }
    }
    return NULL;
}

const GameStatusEffectPrototype *game_content_schema_find_effect(const GameContentPrototypeSet *set, const char *id) {
    if (!set || !id) {
        return NULL;
    }
    for (size_t i = 0u; i < set->effect_count; ++i) {
        if (strcmp(set->effects[i].id, id) == 0) {
            return &set->effects[i];
        }
    }
    return NULL;
}
