#include "core/arena.h"

#include <stdlib.h>

static size_t game_align_forward(size_t value, size_t alignment) {
    size_t mask = alignment - 1U;
    return (value + mask) & ~mask;
}

bool game_arena_init(GameArena *arena, size_t capacity) {
    if (!arena || capacity == 0) {
        return false;
    }

    arena->base = (unsigned char *)malloc(capacity);
    if (!arena->base) {
        return false;
    }

    arena->capacity = capacity;
    arena->offset = 0;
    return true;
}

void game_arena_destroy(GameArena *arena) {
    if (!arena || !arena->base) {
        return;
    }

    free(arena->base);
    arena->base = NULL;
    arena->capacity = 0;
    arena->offset = 0;
}

void *game_arena_push(GameArena *arena, size_t size, size_t alignment) {
    if (!arena || !arena->base || size == 0) {
        return NULL;
    }

    if (alignment == 0) {
        alignment = 1;
    }

    size_t aligned = game_align_forward(arena->offset, alignment);
    size_t next = aligned + size;

    if (next > arena->capacity) {
        return NULL;
    }

    void *ptr = arena->base + aligned;
    arena->offset = next;
    return ptr;
}

void game_arena_reset(GameArena *arena) {
    if (!arena) {
        return;
    }
    arena->offset = 0;
}
