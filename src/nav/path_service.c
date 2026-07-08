#include "nav/path_service.h"

#include <stdbool.h>
#include <stddef.h>

GamePathServiceResult game_path_service_init(
    GamePathService *service,
    GamePathServiceRequestSlot *slots,
    size_t slot_count,
    uint64_t ttl_ticks
) {
    if (!service || !slots || slot_count == 0u) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    service->capacity = slot_count;
    service->slots = slots;
    service->ttl_ticks = ttl_ticks;
    service->current_tick = 0u;

    for (size_t i = 0u; i < slot_count; ++i) {
        slots[i].in_use = false;
        slots[i].version = 1u;
        slots[i].handle.slot = (uint32_t)i;
        slots[i].handle.version = slots[i].version;
        slots[i].state = GAME_PATH_SERVICE_STATE_PENDING;
        slots[i].requested_at_tick = 0u;
        slots[i].expires_at_tick = 0u;
        slots[i].result_version = 0u;
        slots[i].request_budget = 0u;
        slots[i].result_path = NULL;
        slots[i].result_path_capacity = 0u;
        slots[i].result_path_length = 0u;
        slots[i].has_result = false;
        slots[i].last_query_result = GAME_PATH_FIND_RESULT_OK;
        slots[i].cost_map = NULL;
        slots[i].scratch = NULL;
    }

    return GAME_PATH_SERVICE_RESULT_OK;
}

static GamePathServiceResult game_path_service_add_tick_budget(uint64_t base_tick, uint64_t extra_ticks, uint64_t *out_tick) {
    if (!out_tick) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }
    if (UINT64_MAX - base_tick < extra_ticks) {
        *out_tick = UINT64_MAX;
        return GAME_PATH_SERVICE_RESULT_OK;
    }
    *out_tick = base_tick + extra_ticks;
    return GAME_PATH_SERVICE_RESULT_OK;
}

static GamePathServiceState game_path_service_next_state_for_result(GamePathFindResult query_result) {
    if (query_result == GAME_PATH_FIND_RESULT_OK) {
        return GAME_PATH_SERVICE_STATE_RESOLVED;
    }
    if (query_result == GAME_PATH_FIND_RESULT_BUDGET_EXHAUSTED) {
        return GAME_PATH_SERVICE_STATE_PENDING;
    }
    return GAME_PATH_SERVICE_STATE_FAILED;
}

static bool game_path_service_expired(const GamePathServiceRequestSlot *slot, uint64_t current_tick) {
    return current_tick >= slot->expires_at_tick;
}

static bool game_path_service_handle_matches(const GamePathServiceRequestSlot *slot, GamePathRequestHandle handle) {
    return slot->in_use && slot->handle.slot == handle.slot && slot->handle.version == handle.version;
}

static GamePathServiceRequestSlot *game_path_service_lookup_slot(GamePathService *service, GamePathRequestHandle handle) {
    if (!service || !service->slots || handle.slot >= service->capacity) {
        return NULL;
    }

    if (!game_path_service_handle_matches(&service->slots[handle.slot], handle)) {
        return NULL;
    }

    return &service->slots[handle.slot];
}

static const GamePathServiceRequestSlot *game_path_service_lookup_slot_const(
    const GamePathService *service,
    GamePathRequestHandle handle
) {
    if (!service || !service->slots || handle.slot >= service->capacity) {
        return NULL;
    }

    const GamePathServiceRequestSlot *slot = &service->slots[handle.slot];
    if (!slot->in_use || slot->handle.slot != handle.slot || slot->handle.version != handle.version) {
        return NULL;
    }

    return slot;
}

GamePathServiceResult game_path_service_submit(
    GamePathService *service,
    uint64_t current_tick,
    GameHexAxial start,
    GameHexAxial goal,
    const GamePathCostMap *cost_map,
    GamePathQueryScratch *scratch,
    uint32_t request_budget,
    GameHexAxial *result_path_buffer,
    size_t result_path_capacity,
    GamePathRequestHandle *out_handle
) {
    if (!service || !service->slots || !cost_map || !scratch || !result_path_buffer || !out_handle) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    if (request_budget == 0u) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    if (result_path_capacity == 0u) {
        return GAME_PATH_SERVICE_RESULT_PATH_TOO_SMALL;
    }

    for (size_t i = 0u; i < service->capacity; ++i) {
        if (!service->slots[i].in_use) {
            GamePathServiceRequestSlot *slot = &service->slots[i];
            slot->in_use = true;
            slot->handle.slot = (uint32_t)i;
            slot->handle.version = slot->version;
            slot->state = GAME_PATH_SERVICE_STATE_PENDING;
            slot->requested_at_tick = current_tick;
            if (game_path_service_add_tick_budget(current_tick, service->ttl_ticks, &slot->expires_at_tick) !=
                GAME_PATH_SERVICE_RESULT_OK) {
                return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
            }
            slot->result_version = 0u;
            slot->request_budget = request_budget;
            slot->start = start;
            slot->goal = goal;
            slot->cost_map = cost_map;
            slot->scratch = scratch;
            slot->result_path = result_path_buffer;
            slot->result_path_capacity = result_path_capacity;
            slot->result_path_length = 0u;
            slot->has_result = false;
            slot->last_query_result = GAME_PATH_FIND_RESULT_OK;
            *out_handle = slot->handle;

            return GAME_PATH_SERVICE_RESULT_OK;
        }
    }

    return GAME_PATH_SERVICE_RESULT_NO_SLOT;
}

GamePathServiceResult game_path_service_cancel(GamePathService *service, GamePathRequestHandle handle) {
    GamePathServiceRequestSlot *slot = game_path_service_lookup_slot(service, handle);
    if (!slot) {
        return GAME_PATH_SERVICE_RESULT_INVALID_HANDLE;
    }

    if (slot->state == GAME_PATH_SERVICE_STATE_CANCELLED) {
        return GAME_PATH_SERVICE_RESULT_OK;
    }

    if (slot->state != GAME_PATH_SERVICE_STATE_PENDING) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    slot->state = GAME_PATH_SERVICE_STATE_CANCELLED;
    slot->result_version++;
    slot->has_result = false;
    return GAME_PATH_SERVICE_RESULT_OK;
}

GamePathServiceResult game_path_service_status(
    const GamePathService *service,
    GamePathRequestHandle handle,
    GamePathServiceStatus *out_status
) {
    if (!service || !out_status) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    const GamePathServiceRequestSlot *slot = game_path_service_lookup_slot_const(service, handle);
    if (!slot) {
        return GAME_PATH_SERVICE_RESULT_INVALID_HANDLE;
    }

    out_status->state = slot->state;
    out_status->requested_at_tick = slot->requested_at_tick;
    out_status->expires_at_tick = slot->expires_at_tick;
    out_status->result_version = slot->result_version;
    out_status->result_path_length = slot->result_path_length;
    out_status->request_budget = slot->request_budget;
    out_status->last_query_result = slot->last_query_result;
    out_status->has_result_path = slot->has_result;
    return GAME_PATH_SERVICE_RESULT_OK;
}

GamePathServiceResult game_path_service_retrieve(
    const GamePathService *service,
    GamePathRequestHandle handle,
    GameHexAxial *out_path,
    size_t out_capacity,
    size_t *out_length,
    uint64_t *out_result_version
) {
    if (!service || !out_path || !out_length || !out_result_version) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    const GamePathServiceRequestSlot *slot = game_path_service_lookup_slot_const(service, handle);
    if (!slot) {
        return GAME_PATH_SERVICE_RESULT_INVALID_HANDLE;
    }

    if (slot->state != GAME_PATH_SERVICE_STATE_RESOLVED) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    if (!slot->has_result) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    if (out_capacity < slot->result_path_length) {
        return GAME_PATH_SERVICE_RESULT_PATH_TOO_SMALL;
    }

    for (size_t i = 0u; i < slot->result_path_length; ++i) {
        out_path[i] = slot->result_path[i];
    }

    *out_length = slot->result_path_length;
    *out_result_version = slot->result_version;
    return GAME_PATH_SERVICE_RESULT_OK;
}

GamePathServiceResult game_path_service_advance_tick(GamePathService *service, uint64_t current_tick, uint32_t tick_budget) {
    if (!service || !service->slots) {
        return GAME_PATH_SERVICE_RESULT_INVALID_ARGUMENT;
    }

    service->current_tick = current_tick;
    uint32_t remaining_budget = tick_budget;

    for (size_t i = 0u; i < service->capacity; ++i) {
        GamePathServiceRequestSlot *slot = &service->slots[i];
        if (!slot->in_use) {
            continue;
        }

        if (slot->state == GAME_PATH_SERVICE_STATE_PENDING) {
            if (game_path_service_expired(slot, current_tick)) {
                slot->state = GAME_PATH_SERVICE_STATE_EXPIRED;
                slot->result_version++;
                slot->has_result = false;
                continue;
            }

            if (remaining_budget == 0u) {
                continue;
            }

            slot->has_result = false;
            slot->result_path_length = 0u;
            size_t expanded_count = 0u;
            uint32_t node_budget =
                remaining_budget < slot->request_budget ? remaining_budget : slot->request_budget;

            GamePathFindResult query_result = game_pathfind_query(
                slot->start,
                slot->goal,
                slot->cost_map,
                slot->scratch,
                node_budget,
                slot->result_path,
                slot->result_path_capacity,
                &slot->result_path_length,
                &expanded_count
            );
            slot->last_query_result = query_result;
            uint32_t consumed = expanded_count > (size_t)UINT32_MAX ? UINT32_MAX : (uint32_t)expanded_count;
            if (consumed == 0u && query_result != GAME_PATH_FIND_RESULT_INVALID_ARGUMENT) {
                consumed = 1u;
            }
            remaining_budget = consumed >= remaining_budget ? 0u : remaining_budget - consumed;

            GamePathServiceState next_state = game_path_service_next_state_for_result(query_result);
            if (next_state == GAME_PATH_SERVICE_STATE_RESOLVED) {
                slot->state = GAME_PATH_SERVICE_STATE_RESOLVED;
                slot->has_result = true;
                slot->result_version++;
            } else if (next_state == GAME_PATH_SERVICE_STATE_PENDING) {
                slot->state = GAME_PATH_SERVICE_STATE_PENDING;
            } else {
                slot->state = GAME_PATH_SERVICE_STATE_FAILED;
                slot->result_version++;
            }

            continue;
        }

        if (slot->state == GAME_PATH_SERVICE_STATE_RESOLVED || slot->state == GAME_PATH_SERVICE_STATE_FAILED
            || slot->state == GAME_PATH_SERVICE_STATE_CANCELLED) {
            if (game_path_service_expired(slot, current_tick)) {
                slot->state = GAME_PATH_SERVICE_STATE_EXPIRED;
                slot->result_version++;
            }
        }

        if (slot->state == GAME_PATH_SERVICE_STATE_EXPIRED && current_tick > slot->expires_at_tick) {
            slot->in_use = false;
            slot->version++;
            slot->handle.version = slot->version;
            slot->result_path = NULL;
            slot->result_path_capacity = 0u;
            slot->result_path_length = 0u;
            slot->has_result = false;
        }
    }

    return GAME_PATH_SERVICE_RESULT_OK;
}
