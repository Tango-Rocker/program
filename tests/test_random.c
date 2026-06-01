#include "core/random.h"

#include <stdio.h>

static int assert_true(int condition, const char *label) {
    if (!condition) {
        printf("[random] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_random(void) {
    int failed = 0;

    GameRng a = {0};
    GameRng b = {0};
    game_rng_seed(&a, 0x123456789ull);
    game_rng_seed(&b, 0x123456789ull);

    for (int i = 0; i < 16; ++i) {
        failed += assert_true(game_rng_u32(&a) == game_rng_u32(&b), "seeded rng mismatch");
    }

    game_rng_seed(&a, 17);
    float v = game_rng_float01(&a);
    failed += assert_true(v >= 0.0f && v <= 1.0f, "rng float out of range");

    if (failed == 0) {
        printf("[random] PASS\n");
    }

    return failed;
}
