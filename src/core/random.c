#include "core/random.h"

void game_rng_seed(GameRng *rng, uint64_t seed) {
    if (!rng) {
        return;
    }

    rng->state = (seed == 0) ? 0xBAD5EEDu : seed;
}

uint32_t game_rng_u32(GameRng *rng) {
    if (!rng) {
        return 0;
    }

    uint64_t state = rng->state;
    state += 0x9E3779B97F4A7C15ULL;
    rng->state = state;

    state = (state ^ (state >> 30)) * 0xBF58476D1CE4E5B9ULL;
    state = (state ^ (state >> 27)) * 0x94D049BB133111EBULL;
    state = state ^ (state >> 31);

    return (uint32_t)(state & 0xFFFFFFFFu);
}

float game_rng_float01(GameRng *rng) {
    if (!rng) {
        return 0.0f;
    }

    return game_rng_u32(rng) / 4294967295.0f;
}
