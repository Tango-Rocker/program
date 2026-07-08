#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GameArena {
    unsigned char *base;
    size_t capacity;
    size_t offset;
} GameArena;

bool game_arena_init(GameArena *arena, size_t capacity);
void game_arena_destroy(GameArena *arena);
void *game_arena_push(GameArena *arena, size_t size, size_t alignment);
void game_arena_reset(GameArena *arena);

#ifdef __cplusplus
}
#endif
