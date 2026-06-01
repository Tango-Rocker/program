#include <stdio.h>

#include "lua/content_schema.h"

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[content_schema] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static GameContentPrototypeSet make_set(
    GameAbilityPrototype abilities[4],
    GameJobPrototype jobs[4],
    GameStatusEffectPrototype effects[4]
) {
    GameContentPrototypeSet set = {0};
    (void)game_content_schema_init_set(&set, abilities, 4u, jobs, 4u, effects, 4u);
    return set;
}

int test_content_schema(void) {
    int failed = 0;
    GameAbilityPrototype abilities[4] = {0};
    GameJobPrototype jobs[4] = {0};
    GameStatusEffectPrototype effects[4] = {0};
    GameContentPrototypeSet set = make_set(abilities, jobs, effects);
    GameContentSchemaError error = {0};

    failed += assert_true(
        game_content_schema_load("data/lua/prototypes.lua", &set, &error) == GAME_CONTENT_SCHEMA_RESULT_OK,
        "valid content loads"
    );
    failed += assert_true(set.ability_count == 1u && set.job_count == 1u && set.effect_count == 1u, "prototype counts");
    const GameAbilityPrototype *ability = game_content_schema_find_ability(&set, "stone_throw");
    failed += assert_true(ability && ability->damage == 12u, "ability immutable view");

    GameContentPrototypeSet before = set;
    failed += assert_true(
        game_content_schema_load("tests/fixtures/content/missing_job.lua", &set, &error)
            == GAME_CONTENT_SCHEMA_RESULT_MISSING_REQUIRED,
        "missing required field rejected"
    );
    failed += assert_true(set.job_count == before.job_count, "missing required does not partially update");

    failed += assert_true(
        game_content_schema_load("tests/fixtures/content/duplicate_id.lua", &set, &error)
            == GAME_CONTENT_SCHEMA_RESULT_DUPLICATE_ID,
        "duplicate id rejected"
    );
    failed += assert_true(error.line == 2u, "duplicate reports useful line");

    failed += assert_true(
        game_content_schema_load("tests/fixtures/content/range_failure.lua", &set, &error)
            == GAME_CONTENT_SCHEMA_RESULT_OUT_OF_RANGE,
        "range failure rejected"
    );

    if (failed == 0) {
        printf("[content_schema] PASS\n");
    }
    return failed;
}
