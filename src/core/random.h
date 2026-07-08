#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GameRng {
    uint64_t state;
} GameRng;

void game_rng_seed(GameRng *rng, uint64_t seed);
uint32_t game_rng_u32(GameRng *rng);
float game_rng_float01(GameRng *rng);

#ifdef __cplusplus
}
#endif
