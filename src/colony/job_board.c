#include "colony/job_board.h"

#include <limits.h>
#include <stdbool.h>

static bool game_job_board_handle_matches(const GameJobBoardOrderSlot *slot, GameJobOrderHandle handle) {
    return slot->in_use && slot->handle.slot == handle.slot && slot->handle.version == handle.version;
}

static const GameJobBoardOrderSlot *game_job_board_lookup_slot_const(
    const GameJobBoard *board,
    GameJobOrderHandle handle
) {
    if (!board || !board->slots || handle.slot >= board->capacity) {
        return NULL;
    }

    const GameJobBoardOrderSlot *slot = &board->slots[handle.slot];
    if (!game_job_board_handle_matches(slot, handle)) {
        return NULL;
    }
    return slot;
}

static GameJobBoardOrderSlot *game_job_board_lookup_slot(GameJobBoard *board, GameJobOrderHandle handle) {
    if (!board || !board->slots || handle.slot >= board->capacity) {
        return NULL;
    }

    GameJobBoardOrderSlot *slot = &board->slots[handle.slot];
    if (!game_job_board_handle_matches(slot, handle)) {
        return NULL;
    }
    return slot;
}

static bool game_job_board_slot_available_for_create(const GameJobBoardOrderSlot *slot) {
    if (!slot) {
        return false;
    }
    return !slot->in_use || slot->order.state == GAME_JOB_BOARD_STATE_DONE
           || slot->order.state == GAME_JOB_BOARD_STATE_ABORTED;
}

static GameJobBoardResult game_job_board_add_tick(
    uint64_t base_tick,
    uint64_t delta_ticks,
    uint64_t *out_tick
) {
    if (!out_tick) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    if (UINT64_MAX - base_tick < delta_ticks) {
        *out_tick = UINT64_MAX;
        return GAME_JOB_BOARD_RESULT_OK;
    }

    *out_tick = base_tick + delta_ticks;
    return GAME_JOB_BOARD_RESULT_OK;
}

static bool game_job_board_transition_allowed(GameJobBoardOrderState from, GameJobBoardOrderState to) {
    if (from == to) {
        return true;
    }

    switch (from) {
        case GAME_JOB_BOARD_STATE_OPEN:
            return to == GAME_JOB_BOARD_STATE_DONE || to == GAME_JOB_BOARD_STATE_ABORTED;
        case GAME_JOB_BOARD_STATE_RESERVED:
            return to == GAME_JOB_BOARD_STATE_IN_PROGRESS || to == GAME_JOB_BOARD_STATE_DONE
                   || to == GAME_JOB_BOARD_STATE_ABORTED;
        case GAME_JOB_BOARD_STATE_IN_PROGRESS:
            return to == GAME_JOB_BOARD_STATE_STALLED || to == GAME_JOB_BOARD_STATE_DONE || to == GAME_JOB_BOARD_STATE_ABORTED;
        case GAME_JOB_BOARD_STATE_STALLED:
            return to == GAME_JOB_BOARD_STATE_OPEN || to == GAME_JOB_BOARD_STATE_IN_PROGRESS
                   || to == GAME_JOB_BOARD_STATE_ABORTED;
        case GAME_JOB_BOARD_STATE_DONE:
        case GAME_JOB_BOARD_STATE_ABORTED:
            return false;
        default:
            return false;
    }
}

GameJobBoardResult game_job_board_init(
    GameJobBoard *board,
    GameJobBoardOrderSlot *slots,
    size_t slot_count,
    uint64_t reservation_ttl_ticks
) {
    if (!board || !slots || slot_count == 0u) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    board->capacity = slot_count;
    board->slots = slots;
    board->reservation_ttl_ticks = reservation_ttl_ticks;
    board->current_tick = 0u;
    board->next_stable_id = 1u;

    for (size_t i = 0u; i < slot_count; ++i) {
        slots[i].in_use = false;
        slots[i].version = 1u;
        slots[i].handle.slot = (uint32_t)i;
        slots[i].handle.version = slots[i].version;
        slots[i].stable_id = 0u;
        slots[i].order.stable_id = 0u;
        slots[i].order.state = GAME_JOB_BOARD_STATE_OPEN;
        slots[i].order.reserved = false;
        slots[i].order.reserved_by = game_entity_invalid_id();
        slots[i].order.required_role_flags = 0u;
        slots[i].order.required_resource_flags = 0u;
        slots[i].order.duration_min_ticks = 0u;
        slots[i].order.duration_max_ticks = 0u;
        slots[i].order.target = (GameHexAxial){0, 0};
        slots[i].order.expires_at_tick = 0u;
        slots[i].reserved_at_tick = 0u;
        slots[i].requested_at_tick = 0u;
    }

    return GAME_JOB_BOARD_RESULT_OK;
}

size_t game_job_board_capacity(const GameJobBoard *board) {
    return board ? board->capacity : 0u;
}

GameJobBoardResult game_job_board_create_order(
    GameJobBoard *board,
    uint64_t current_tick,
    GameHexAxial target,
    uint32_t required_role_flags,
    uint32_t required_resource_flags,
    uint32_t duration_min_ticks,
    uint32_t duration_max_ticks,
    GameJobOrderHandle *out_handle
) {
    if (!board || !board->slots || !out_handle) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    if (duration_max_ticks != 0u && duration_min_ticks > duration_max_ticks) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    for (size_t i = 0u; i < board->capacity; ++i) {
        GameJobBoardOrderSlot *slot = &board->slots[i];
        if (!game_job_board_slot_available_for_create(slot)) {
            continue;
        }

        uint32_t stable_id = board->next_stable_id;
        if (stable_id == 0u) {
            return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
        }

        if (slot->in_use) {
            ++slot->version;
            if (slot->version == 0u) {
                slot->version = 1u;
            }
        }
        slot->in_use = true;
        slot->handle.slot = (uint32_t)i;
        slot->handle.version = slot->version;
        slot->stable_id = stable_id;
        slot->requested_at_tick = current_tick;
        slot->reserved_at_tick = 0u;
        slot->order.stable_id = stable_id;
        slot->order.target = target;
        slot->order.required_role_flags = required_role_flags;
        slot->order.required_resource_flags = required_resource_flags;
        slot->order.duration_min_ticks = duration_min_ticks;
        slot->order.duration_max_ticks = duration_max_ticks;
        slot->order.state = GAME_JOB_BOARD_STATE_OPEN;
        slot->order.reserved_by = game_entity_invalid_id();
        slot->order.reserved = false;
        slot->order.expires_at_tick = 0u;
        *out_handle = slot->handle;
        ++board->next_stable_id;
        return GAME_JOB_BOARD_RESULT_OK;
    }

    return GAME_JOB_BOARD_RESULT_NO_SLOT;
}

static void game_job_board_clear_reservation(GameJobBoardOrderSlot *slot) {
    if (!slot) {
        return;
    }

    if (slot->order.state == GAME_JOB_BOARD_STATE_RESERVED || slot->order.state == GAME_JOB_BOARD_STATE_STALLED) {
        if (slot->order.state == GAME_JOB_BOARD_STATE_RESERVED) {
            slot->order.state = GAME_JOB_BOARD_STATE_OPEN;
        }
    }
    slot->order.reserved = false;
    slot->order.reserved_by = game_entity_invalid_id();
    slot->order.expires_at_tick = 0u;
    slot->reserved_at_tick = 0u;
}

GameJobBoardResult game_job_board_reserve(
    GameJobBoard *board,
    GameJobOrderHandle handle,
    uint64_t current_tick,
    GameEntityId worker
) {
    if (!board || !board->slots) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    if (!game_entity_id_is_valid(worker)) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    GameJobBoardOrderSlot *slot = game_job_board_lookup_slot(board, handle);
    if (!slot) {
        return GAME_JOB_BOARD_RESULT_INVALID_HANDLE;
    }

    if (slot->order.state == GAME_JOB_BOARD_STATE_RESERVED) {
        if (game_entity_id_is_valid(slot->order.reserved_by)
            && slot->order.reserved_by.index == worker.index && slot->order.reserved_by.generation == worker.generation) {
            slot->reserved_at_tick = current_tick;
            (void)game_job_board_add_tick(current_tick, board->reservation_ttl_ticks, &slot->order.expires_at_tick);
            return GAME_JOB_BOARD_RESULT_OK;
        }
        return GAME_JOB_BOARD_RESULT_ALREADY_RESERVED;
    }

    if (slot->order.state != GAME_JOB_BOARD_STATE_OPEN) {
        return GAME_JOB_BOARD_RESULT_INVALID_STATE;
    }

    slot->order.state = GAME_JOB_BOARD_STATE_RESERVED;
    slot->order.reserved = true;
    slot->order.reserved_by = worker;
    slot->reserved_at_tick = current_tick;
    GameJobBoardResult expiry = game_job_board_add_tick(current_tick, board->reservation_ttl_ticks, &slot->order.expires_at_tick);
    if (expiry != GAME_JOB_BOARD_RESULT_OK) {
        return expiry;
    }
    return GAME_JOB_BOARD_RESULT_OK;
}

GameJobBoardResult game_job_board_transition(
    GameJobBoard *board,
    GameJobOrderHandle handle,
    GameJobBoardOrderState next_state
) {
    if (!board || !board->slots) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    GameJobBoardOrderSlot *slot = game_job_board_lookup_slot(board, handle);
    if (!slot) {
        return GAME_JOB_BOARD_RESULT_INVALID_HANDLE;
    }

    if (!game_job_board_transition_allowed(slot->order.state, next_state)) {
        return GAME_JOB_BOARD_RESULT_INVALID_STATE;
    }

    if (next_state == GAME_JOB_BOARD_STATE_IN_PROGRESS && !game_entity_id_is_valid(slot->order.reserved_by)) {
        return GAME_JOB_BOARD_RESULT_INVALID_STATE;
    }

    slot->order.state = next_state;
    if (next_state == GAME_JOB_BOARD_STATE_RESERVED) {
        slot->order.reserved = true;
    } else if (next_state == GAME_JOB_BOARD_STATE_OPEN) {
        slot->order.state = GAME_JOB_BOARD_STATE_OPEN;
        game_job_board_clear_reservation(slot);
    } else if (next_state == GAME_JOB_BOARD_STATE_DONE || next_state == GAME_JOB_BOARD_STATE_ABORTED) {
        slot->order.reserved = false;
        slot->order.reserved_by = game_entity_invalid_id();
        slot->order.expires_at_tick = 0u;
        slot->reserved_at_tick = 0u;
    }

    return GAME_JOB_BOARD_RESULT_OK;
}

GameJobBoardResult game_job_board_advance_tick(GameJobBoard *board, uint64_t current_tick) {
    if (!board || !board->slots) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    board->current_tick = current_tick;

    for (size_t i = 0u; i < board->capacity; ++i) {
        GameJobBoardOrderSlot *slot = &board->slots[i];
        if (!slot->in_use) {
            continue;
        }

        if (slot->order.state != GAME_JOB_BOARD_STATE_RESERVED) {
            continue;
        }

        if (slot->order.expires_at_tick != 0u && current_tick >= slot->order.expires_at_tick) {
            game_job_board_clear_reservation(slot);
        }
    }

    return GAME_JOB_BOARD_RESULT_OK;
}

GameJobBoardResult game_job_board_release_reservation(GameJobBoard *board, GameJobOrderHandle handle) {
    if (!board || !board->slots) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    GameJobBoardOrderSlot *slot = game_job_board_lookup_slot(board, handle);
    if (!slot) {
        return GAME_JOB_BOARD_RESULT_INVALID_HANDLE;
    }

    if (slot->order.state == GAME_JOB_BOARD_STATE_DONE || slot->order.state == GAME_JOB_BOARD_STATE_ABORTED) {
        return GAME_JOB_BOARD_RESULT_INVALID_STATE;
    }

    game_job_board_clear_reservation(slot);
    return GAME_JOB_BOARD_RESULT_OK;
}

GameJobBoardResult game_job_board_status(
    const GameJobBoard *board,
    GameJobOrderHandle handle,
    GameJobOrder *out_order
) {
    if (!out_order || !board || !board->slots) {
        return GAME_JOB_BOARD_RESULT_INVALID_ARGUMENT;
    }

    const GameJobBoardOrderSlot *slot = game_job_board_lookup_slot_const(board, handle);
    if (!slot) {
        return GAME_JOB_BOARD_RESULT_INVALID_HANDLE;
    }

    *out_order = slot->order;
    return GAME_JOB_BOARD_RESULT_OK;
}
