#include "render/hex_debug_render.h"

typedef struct {
    GameHexAxial selected;
    bool has_selected;
    GameHexDebugCell *out_cells;
    size_t capacity;
    size_t count;
    GameHexDebugRenderResult result;
} GameHexDebugCollectContext;

static bool game_hex_debug_tile_equal(GameHexAxial a, GameHexAxial b) {
    return a.q == b.q && a.r == b.r;
}

GameDebugColor game_hex_debug_color_for_tile(int32_t tile_value, bool selected, bool highlighted) {
    if (selected) {
        return (GameDebugColor){255u, 230u, 120u, 255u};
    }
    if (highlighted) {
        return (GameDebugColor){100u, 190u, 255u, 220u};
    }
    if (tile_value < 0) {
        return (GameDebugColor){80u, 84u, 92u, 255u};
    }
    return (GameDebugColor){72u, 150u, 96u, 255u};
}

static void game_hex_debug_collect_visit(void *user, GameWorldMapTileVisit visit) {
    GameHexDebugCollectContext *context = (GameHexDebugCollectContext *)user;
    if (!context || context->result != GAME_HEX_DEBUG_RENDER_RESULT_OK) {
        return;
    }
    if (context->count >= context->capacity) {
        context->result = GAME_HEX_DEBUG_RENDER_RESULT_BUFFER_TOO_SMALL;
        return;
    }

    bool selected = context->has_selected && game_hex_debug_tile_equal(context->selected, visit.tile);
    bool highlighted = visit.value > 1;
    context->out_cells[context->count++] = (GameHexDebugCell){
        .tile = visit.tile,
        .tile_value = visit.value,
        .color = game_hex_debug_color_for_tile(visit.value, selected, highlighted),
        .blocked = visit.value < 0,
        .selected = selected,
        .highlighted = highlighted,
    };
}

GameHexDebugRenderResult game_hex_debug_render_collect(
    const GameWorldMap *map,
    GameHexAxial selected,
    bool has_selected,
    GameHexDebugCell *out_cells,
    size_t capacity,
    size_t *out_count
) {
    if (!map || !out_cells || capacity == 0u || !out_count) {
        return GAME_HEX_DEBUG_RENDER_RESULT_INVALID_ARGUMENT;
    }

    GameHexDebugCollectContext context = {
        .selected = selected,
        .has_selected = has_selected,
        .out_cells = out_cells,
        .capacity = capacity,
        .count = 0u,
        .result = GAME_HEX_DEBUG_RENDER_RESULT_OK,
    };

    if (game_world_map_for_each_tile(map, game_hex_debug_collect_visit, &context) != GAME_WORLD_MAP_RESULT_OK) {
        return GAME_HEX_DEBUG_RENDER_RESULT_INVALID_ARGUMENT;
    }

    *out_count = context.count;
    return context.result;
}
