#include "ecs/entity.h"

#include <limits.h>
#include <stdio.h>

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[ecs] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int assert_equal_u32(uint32_t lhs, uint32_t rhs, const char *label) {
    if (lhs != rhs) {
        printf("[ecs] FAIL: %s (%u != %u)\n", label, lhs, rhs);
        return 1;
    }
    return 0;
}

int test_entity_registry(void) {
    int failed = 0;

    GameEntityRegistry registry = {0};
    GameEntityId a = {0};
    GameEntityId b = {0};
    GameEntityId c = {0};
    GameEntityId reuse = {0};
    GameEntityResult result = GAME_ENTITY_RESULT_OK;

    result = game_entity_registry_init(&registry, 3);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "entity registry init");
    failed += assert_true(game_entity_registry_capacity(&registry) == 3, "entity capacity");

    result = game_entity_registry_create(&registry, &a);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "create a");
    failed += assert_true(game_entity_registry_is_alive(&registry, a), "a alive");
    failed += assert_equal_u32(a.generation, 0, "a generation starts at 0");
    failed += assert_equal_u32(a.index, 0, "a index 0");

    result = game_entity_registry_create(&registry, &b);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "create b");
    failed += assert_true(game_entity_registry_is_alive(&registry, b), "b alive");
    failed += assert_equal_u32(b.index, 1, "b index 1");

    result = game_entity_registry_create(&registry, &c);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "create c");
    failed += assert_equal_u32(c.index, 2, "c index 2");

    result = game_entity_registry_create(&registry, &reuse);
    failed += assert_true(result == GAME_ENTITY_RESULT_POOL_FULL, "create beyond capacity");

    result = game_entity_registry_destroy_id(&registry, a);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "destroy a");
    failed += assert_true(!game_entity_registry_is_alive(&registry, a), "a not alive after destroy");

    result = game_entity_registry_create(&registry, &reuse);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "reuse slot after one destroy");
    failed += assert_equal_u32(reuse.index, 0, "reused index 0");
    failed += assert_equal_u32(reuse.generation, 1, "reused generation increments");

    result = game_entity_registry_destroy_id(&registry, a);
    failed += assert_true(result == GAME_ENTITY_RESULT_STALE_HANDLE, "destroy stale handle after reuse");
    failed += assert_true(!game_entity_registry_is_alive(&registry, a), "stale handle not alive");
    result = game_entity_registry_destroy_id(&registry, reuse);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "destroy reused a");

    result = game_entity_registry_destroy_id(&registry, b);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "destroy b");
    result = game_entity_registry_destroy_id(&registry, c);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "destroy c");

    result = game_entity_registry_create(&registry, &a);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "create after b,c destroy");
    result = game_entity_registry_create(&registry, &b);
    failed += assert_true(result == GAME_ENTITY_RESULT_OK, "create next after b,c destroy");

    failed += assert_true(a.index == 0, "reuse order is deterministic (0 before 1)");
    failed += assert_true(b.index == 1, "reuse order is deterministic (1 before 2)");

    game_entity_registry_destroy(&registry);
    if (failed == 0) {
        printf("[ecs] PASS\n");
    }
    return failed;
}
