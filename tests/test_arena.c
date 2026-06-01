#include "core/arena.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[arena] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

static int check_alignment(void *ptr, size_t align) {
    return ((uintptr_t)ptr % align) == 0 ? 0 : 1;
}

int test_arena(void) {
    int failed = 0;
    GameArena a = {0};

    failed += assert_true(game_arena_init(&a, 64), "arena init failed");

    void *p1 = game_arena_push(&a, 1, 8);
    failed += assert_true(p1 != NULL, "arena first push null");
    failed += assert_true(check_alignment(p1, 8) == 0, "first push alignment");

    void *p2 = game_arena_push(&a, 16, 16);
    failed += assert_true(p2 != NULL, "arena second push null");
    failed += assert_true(check_alignment(p2, 16) == 0, "second push alignment");

    GameArena small = {0};
    failed += assert_true(game_arena_init(&small, 8), "small arena init failed");
    void *p3 = game_arena_push(&small, 16, 8);
    failed += assert_true(p3 == NULL, "arena overflow not detected");
    game_arena_destroy(&small);

    game_arena_reset(&a);
    void *p4 = game_arena_push(&a, 8, 8);
    failed += assert_true(p4 != NULL, "arena reset did not free space");

    game_arena_destroy(&a);

    if (failed == 0) {
        printf("[arena] PASS\n");
    }

    return failed;
}
