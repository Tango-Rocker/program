#include "sim/snapshot.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sim/sim_context.h"

static const uint8_t GAME_SNAPSHOT_MAGIC[4] = {'G', 'S', 'N', 'P'};

typedef struct {
    uint8_t *cursor;
    size_t used;
    size_t capacity;
} SnapshotWriter;

typedef struct {
    const uint8_t *cursor;
    size_t used;
    size_t capacity;
} SnapshotReader;

static bool writer_write_bytes(SnapshotWriter *writer, const void *data, size_t size) {
    if (!writer || !data || writer->used + size > writer->capacity) {
        return false;
    }
    memcpy(writer->cursor + writer->used, data, size);
    writer->used += size;
    return true;
}

static bool writer_write_u8(SnapshotWriter *writer, uint8_t value) {
    return writer_write_bytes(writer, &value, sizeof(value));
}

static bool writer_write_u16(SnapshotWriter *writer, uint16_t value) {
    return writer_write_bytes(writer, &value, sizeof(value));
}

static bool writer_write_u32(SnapshotWriter *writer, uint32_t value) {
    return writer_write_bytes(writer, &value, sizeof(value));
}

static bool writer_write_u64(SnapshotWriter *writer, uint64_t value) {
    return writer_write_bytes(writer, &value, sizeof(value));
}

static bool writer_write_i32(SnapshotWriter *writer, int32_t value) {
    return writer_write_bytes(writer, &value, sizeof(value));
}

static bool reader_read_bytes(SnapshotReader *reader, void *out_data, size_t size) {
    if (!reader || !out_data || reader->used + size > reader->capacity) {
        return false;
    }
    memcpy(out_data, reader->cursor + reader->used, size);
    reader->used += size;
    return true;
}

static bool reader_read_u8(SnapshotReader *reader, uint8_t *value) {
    return reader_read_bytes(reader, value, sizeof(*value));
}

static bool reader_read_u16(SnapshotReader *reader, uint16_t *value) {
    return reader_read_bytes(reader, value, sizeof(*value));
}

static bool reader_read_u32(SnapshotReader *reader, uint32_t *value) {
    return reader_read_bytes(reader, value, sizeof(*value));
}

static bool reader_read_u64(SnapshotReader *reader, uint64_t *value) {
    return reader_read_bytes(reader, value, sizeof(*value));
}

static bool reader_read_i32(SnapshotReader *reader, int32_t *value) {
    return reader_read_bytes(reader, value, sizeof(*value));
}

static size_t game_snapshot_scheduler_entry_size(void) {
    return sizeof(uint32_t) * 2u              /* slot, generation */
           + sizeof(uint64_t) * 2u             /* due_tick, sequence */
           + sizeof(uint8_t) * 2u              /* type, payload_size */
           + sizeof(uint16_t)                  /* reserved */
           + sizeof(uint32_t) * 2u              /* source id */
           + GAME_SCHEDULER_MAX_PAYLOAD_BYTES;
}

size_t game_snapshot_required_size(
    const GameSimContext *context,
    const GameWorldMap *world_map,
    const GameScheduler *scheduler
) {
    if (!context || !world_map || !scheduler) {
        return 0u;
    }

    if (world_map->chunk_radius <= 0) {
        return 0u;
    }

    if (world_map->chunk_radius > 2000) {
        return 0u;
    }

    uint64_t tile_count = (uint64_t)world_map->chunk_radius
                         * (uint64_t)world_map->chunk_radius
                         * (uint64_t)world_map->chunk_radius;

    size_t map_size = sizeof(int32_t) * 2u             /* chunk radius, default tile */
                      + sizeof(uint64_t)               /* chunk count */
                      + sizeof(uint64_t) * world_map->chunk_count /* per-chunk tile count */
                      + world_map->chunk_count * (sizeof(int32_t) * 3u + tile_count * sizeof(int32_t));

    size_t scheduler_size = game_snapshot_scheduler_entry_size() * game_scheduler_active_count(scheduler);

    size_t entity_data_size = sizeof(uint32_t) * 4u  /* next_index, free_head, free_tail, free_count */
                             + sizeof(uint32_t) * context->entity_registry.capacity
                             + sizeof(uint8_t) * context->entity_registry.capacity
                             + sizeof(uint32_t) * context->entity_registry.free_count;

    return sizeof(GAME_SNAPSHOT_MAGIC)
           + sizeof(uint16_t) * 2u
           + sizeof(uint64_t)                             /* tick */
           + sizeof(uint64_t)                             /* rng state */
           + sizeof(uint32_t)                             /* entity capacity */
           + sizeof(uint32_t)                             /* event capacity */
           + sizeof(uint32_t)                             /* scheduler capacity */
           + sizeof(uint64_t)                             /* scheduler sequence */
           + sizeof(uint64_t)                             /* active scheduler count */
           + entity_data_size
           + map_size
           + scheduler_size;
}

GameSnapshotResult game_snapshot_serialize(
    const GameSimContext *context,
    const GameWorldMap *world_map,
    const GameScheduler *scheduler,
    uint8_t *out_buffer,
    size_t out_capacity,
    size_t *out_size
) {
    if (!context || !world_map || !scheduler || !out_buffer || !out_size) {
        return GAME_SNAPSHOT_RESULT_INVALID_ARGUMENT;
    }

    if (!context->initialized || !context->entity_registry.generations || !world_map->chunks || !scheduler->entries) {
        return GAME_SNAPSHOT_RESULT_INVALID_ARGUMENT;
    }

    size_t required = game_snapshot_required_size(context, world_map, scheduler);
    if (required == 0u || out_capacity < required) {
        return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
    }

    SnapshotWriter writer = {.cursor = out_buffer, .used = 0u, .capacity = out_capacity};

    if (!writer_write_bytes(&writer, GAME_SNAPSHOT_MAGIC, sizeof(GAME_SNAPSHOT_MAGIC))
        || !writer_write_u16(&writer, GAME_SNAPSHOT_VERSION)
        || !writer_write_u16(&writer, 0u)) {
        return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
    }

    if (!writer_write_u64(&writer, context->tick)
        || !writer_write_u64(&writer, context->rng.state)
        || !writer_write_u32(&writer, (uint32_t)context->entity_registry.capacity)
        || !writer_write_u32(&writer, (uint32_t)context->entity_registry.next_index)
        || !writer_write_u32(&writer, (uint32_t)context->entity_registry.free_head)
        || !writer_write_u32(&writer, (uint32_t)context->entity_registry.free_tail)
        || !writer_write_u32(&writer, (uint32_t)context->entity_registry.free_count)
        || !writer_write_u32(&writer, (uint32_t)context->event_queue.capacity)
        || !writer_write_u32(&writer, (uint32_t)scheduler->capacity)
        || !writer_write_u64(&writer, scheduler->next_sequence)
        || !writer_write_u64(&writer, (uint64_t)game_scheduler_active_count(scheduler))) {
        return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
    }

    for (uint32_t i = 0u; i < context->entity_registry.capacity; ++i) {
        if (!writer_write_u32(&writer, context->entity_registry.generations[i])
            || !writer_write_u8(
                   &writer,
                   context->entity_registry.alive[i] ? 1u : 0u)) {
            return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
        }
    }

    for (uint32_t i = 0u; i < context->entity_registry.free_count; ++i) {
        if (!writer_write_u32(&writer, context->entity_registry.free_indices[i])) {
            return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
        }
    }

    if (!writer_write_i32(&writer, world_map->chunk_radius)
        || !writer_write_i32(&writer, world_map->default_tile_value)
        || !writer_write_u64(&writer, (uint64_t)world_map->chunk_count)) {
        return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
    }

    uint64_t tile_count = (uint64_t)world_map->chunk_radius
                          * (uint64_t)world_map->chunk_radius
                          * (uint64_t)world_map->chunk_radius;
    for (size_t chunk_index = 0u; chunk_index < world_map->chunk_count; ++chunk_index) {
        const GameWorldMapChunk *chunk = &world_map->chunks[chunk_index];
        if (!chunk || !chunk->tile_values) {
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        if (!writer_write_i32(&writer, chunk->key.cx)
            || !writer_write_i32(&writer, chunk->key.cy)
            || !writer_write_i32(&writer, chunk->key.cz)
            || !writer_write_u64(&writer, tile_count)) {
            return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
        }

        for (uint64_t local_index = 0u; local_index < tile_count; ++local_index) {
            if (!writer_write_i32(&writer, chunk->tile_values[(size_t)local_index])) {
                return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
            }
        }
    }

    for (size_t chunk_index = 0u; chunk_index < scheduler->capacity; ++chunk_index) {
        const GameSchedulerEntry *entry = &scheduler->entries[chunk_index];
        if (!entry->occupied || entry->cancelled || entry->fired) {
            continue;
        }

        if (!writer_write_u32(&writer, (uint32_t)chunk_index)
            || !writer_write_u32(&writer, entry->generation)
            || !writer_write_u64(&writer, entry->due_tick)
            || !writer_write_u64(&writer, entry->sequence)
            || !writer_write_u8(&writer, entry->type)
            || !writer_write_u8(&writer, entry->payload_size)
            || !writer_write_u16(&writer, entry->reserved)
            || !writer_write_u32(&writer, entry->source.index)
            || !writer_write_u32(&writer, entry->source.generation)
            || !writer_write_bytes(&writer, entry->payload, GAME_SCHEDULER_MAX_PAYLOAD_BYTES)) {
            return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
        }
    }

    if (writer.used > out_capacity) {
        return GAME_SNAPSHOT_RESULT_BUFFER_TOO_SMALL;
    }

    *out_size = writer.used;
    return GAME_SNAPSHOT_RESULT_OK;
}

static GameSnapshotResult game_snapshot_read_header(SnapshotReader *reader) {
    uint8_t magic[4];
    if (!reader_read_bytes(reader, magic, sizeof(magic))) {
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }
    if (memcmp(magic, GAME_SNAPSHOT_MAGIC, sizeof(magic)) != 0) {
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }

    uint16_t version = 0u;
    uint16_t reserved = 0u;
    if (!reader_read_u16(reader, &version) || !reader_read_u16(reader, &reserved)) {
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }

    if (version != GAME_SNAPSHOT_VERSION) {
        return GAME_SNAPSHOT_RESULT_UNSUPPORTED_VERSION;
    }

    (void)reserved;
    return GAME_SNAPSHOT_RESULT_OK;
}

static bool game_snapshot_validate_header_bounds(
    uint32_t entity_capacity,
    uint32_t event_queue_capacity,
    uint32_t scheduler_capacity,
    uint32_t entity_next_index,
    uint32_t free_head,
    uint32_t free_tail,
    uint32_t free_count
) {
    if (entity_capacity == 0u || event_queue_capacity == 0u || scheduler_capacity == 0u) {
        return false;
    }

    if (entity_next_index > entity_capacity) {
        return false;
    }

    if (free_count > entity_capacity) {
        return false;
    }

    if (free_count == 0u) {
        return free_head == 0u && free_tail == 0u;
    }

    if (free_head >= entity_capacity || free_tail >= entity_capacity) {
        return false;
    }

    return true;
}

GameSnapshotResult game_snapshot_deserialize(
    const uint8_t *buffer,
    size_t buffer_size,
    const GameSnapshotLoadConfig *config,
    GameSimContext *out_context,
    GameWorldMap *out_world_map,
    GameScheduler *out_scheduler
) {
    if (!buffer || buffer_size == 0u || !out_context || !out_world_map || !out_scheduler) {
        return GAME_SNAPSHOT_RESULT_INVALID_ARGUMENT;
    }

    if (out_context->initialized) {
        game_sim_context_shutdown(out_context);
    }

    if (out_scheduler->entries) {
        game_scheduler_destroy(out_scheduler);
    }

    if (out_world_map->chunks) {
        game_world_map_destroy(out_world_map);
    }

    SnapshotReader reader = {.cursor = buffer, .used = 0u, .capacity = buffer_size};
    GameSnapshotResult header_result = game_snapshot_read_header(&reader);
    if (header_result != GAME_SNAPSHOT_RESULT_OK) {
        return header_result;
    }

    uint64_t tick = 0u;
    uint64_t rng_state = 0u;
    uint32_t registry_capacity = 0u;
    uint32_t registry_next_index = 0u;
    uint32_t registry_free_head = 0u;
    uint32_t registry_free_tail = 0u;
    uint32_t registry_free_count = 0u;
    uint32_t event_capacity = 0u;
    uint32_t scheduler_capacity = 0u;
    uint64_t scheduler_next_sequence = 0u;
    uint64_t scheduler_active_count = 0u;

    if (!reader_read_u64(&reader, &tick)
        || !reader_read_u64(&reader, &rng_state)
        || !reader_read_u32(&reader, &registry_capacity)
        || !reader_read_u32(&reader, &registry_next_index)
        || !reader_read_u32(&reader, &registry_free_head)
        || !reader_read_u32(&reader, &registry_free_tail)
        || !reader_read_u32(&reader, &registry_free_count)
        || !reader_read_u32(&reader, &event_capacity)
        || !reader_read_u32(&reader, &scheduler_capacity)
        || !reader_read_u64(&reader, &scheduler_next_sequence)
        || !reader_read_u64(&reader, &scheduler_active_count)) {
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }

    if (!game_snapshot_validate_header_bounds(
            registry_capacity,
            event_capacity,
            scheduler_capacity,
            registry_next_index,
            registry_free_head,
            registry_free_tail,
            registry_free_count)) {
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }

    if (config) {
        if (config->scheduler_capacity != 0u && config->scheduler_capacity < (size_t)scheduler_capacity) {
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }
        if (config->event_queue_capacity != 0u && config->event_queue_capacity < (size_t)event_capacity) {
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }
    }

    GameSimContextConfig context_config = {
        .rng_seed = rng_state,
        .initial_entity_capacity = registry_capacity,
        .initial_event_capacity = event_capacity,
    };

    if (game_sim_context_init(out_context, &context_config) != GAME_SIM_CONTEXT_RESULT_OK) {
        return GAME_SNAPSHOT_RESULT_INIT_FAILED;
    }
    out_context->tick = tick;

    out_context->entity_registry.next_index = registry_next_index;
    out_context->entity_registry.free_head = registry_free_head;
    out_context->entity_registry.free_tail = registry_free_tail;
    out_context->entity_registry.free_count = registry_free_count;

    for (uint32_t i = 0u; i < registry_capacity; ++i) {
        if (!reader_read_u32(&reader, &out_context->entity_registry.generations[i])) {
            game_sim_context_shutdown(out_context);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        uint8_t alive = 0u;
        if (!reader_read_u8(&reader, &alive)) {
            game_sim_context_shutdown(out_context);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }
        out_context->entity_registry.alive[i] = alive != 0u;
    }

    for (uint32_t i = 0u; i < registry_free_count; ++i) {
        if (!reader_read_u32(&reader, &out_context->entity_registry.free_indices[i])) {
            game_sim_context_shutdown(out_context);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }
    }

    int32_t chunk_radius = 0;
    int32_t default_tile = 0;
    uint64_t chunk_count = 0u;
    if (!reader_read_i32(&reader, &chunk_radius)
        || !reader_read_i32(&reader, &default_tile)
        || !reader_read_u64(&reader, &chunk_count)) {
        game_sim_context_shutdown(out_context);
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }

    if (chunk_radius <= 0) {
        game_sim_context_shutdown(out_context);
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }

    if (chunk_count > 1024u) {
        game_sim_context_shutdown(out_context);
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }

    if (game_world_map_init(out_world_map, chunk_radius, default_tile) != GAME_WORLD_MAP_RESULT_OK) {
        game_sim_context_shutdown(out_context);
        return GAME_SNAPSHOT_RESULT_MALFORMED;
    }

    uint64_t expected_tile_count = (uint64_t)chunk_radius * (uint64_t)chunk_radius * (uint64_t)chunk_radius;
    for (uint64_t chunk_index = 0u; chunk_index < chunk_count; ++chunk_index) {
        GameHexChunkKey key = {0};
        uint64_t tile_count = 0u;

        if (!reader_read_i32(&reader, &key.cx)
            || !reader_read_i32(&reader, &key.cy)
            || !reader_read_i32(&reader, &key.cz)
            || !reader_read_u64(&reader, &tile_count)) {
            game_world_map_destroy(out_world_map);
            game_sim_context_shutdown(out_context);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        if (tile_count != expected_tile_count) {
            game_world_map_destroy(out_world_map);
            game_sim_context_shutdown(out_context);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        if (game_world_map_create_chunk(out_world_map, key) != GAME_WORLD_MAP_RESULT_OK) {
            game_world_map_destroy(out_world_map);
            game_sim_context_shutdown(out_context);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        for (uint64_t local_index = 0u; local_index < tile_count; ++local_index) {
            int32_t value = 0;
            if (!reader_read_i32(&reader, &value)) {
                game_world_map_destroy(out_world_map);
                game_sim_context_shutdown(out_context);
                return GAME_SNAPSHOT_RESULT_MALFORMED;
            }

            if (game_world_map_set_local(
                    out_world_map,
                    key,
                    (GameHexChunkLocal){
                        .lx = (int32_t)(local_index / (uint64_t)(chunk_radius * chunk_radius)),
                        .ly = (int32_t)((local_index / (uint64_t)chunk_radius) % (uint64_t)chunk_radius),
                        .lz = (int32_t)(local_index % (uint64_t)chunk_radius),
                    },
                    value) != GAME_WORLD_MAP_RESULT_OK) {
                game_world_map_destroy(out_world_map);
                game_sim_context_shutdown(out_context);
                return GAME_SNAPSHOT_RESULT_MALFORMED;
            }
        }
    }

    if (game_scheduler_init(out_scheduler, (size_t)scheduler_capacity) != GAME_SCHEDULER_RESULT_OK) {
        game_world_map_destroy(out_world_map);
        game_sim_context_shutdown(out_context);
        return GAME_SNAPSHOT_RESULT_INIT_FAILED;
    }
    out_scheduler->next_sequence = scheduler_next_sequence;

    for (uint64_t i = 0u; i < scheduler_active_count; ++i) {
        uint32_t slot = 0u;
        uint32_t generation = 0u;
        uint64_t due_tick = 0u;
        uint64_t sequence = 0u;
        uint8_t type = 0u;
        uint8_t payload_size = 0u;
        uint16_t reserved = 0u;
        uint32_t source_index = 0u;
        uint32_t source_generation = 0u;

        if (!reader_read_u32(&reader, &slot)
            || !reader_read_u32(&reader, &generation)
            || !reader_read_u64(&reader, &due_tick)
            || !reader_read_u64(&reader, &sequence)
            || !reader_read_u8(&reader, &type)
            || !reader_read_u8(&reader, &payload_size)
            || !reader_read_u16(&reader, &reserved)
            || !reader_read_u32(&reader, &source_index)
            || !reader_read_u32(&reader, &source_generation)) {
            game_sim_context_shutdown(out_context);
            game_scheduler_destroy(out_scheduler);
            game_world_map_destroy(out_world_map);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        if (slot >= out_scheduler->capacity) {
            game_sim_context_shutdown(out_context);
            game_scheduler_destroy(out_scheduler);
            game_world_map_destroy(out_world_map);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        if (payload_size > GAME_SCHEDULER_MAX_PAYLOAD_BYTES) {
            game_sim_context_shutdown(out_context);
            game_scheduler_destroy(out_scheduler);
            game_world_map_destroy(out_world_map);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        GameSchedulerEntry *entry = &out_scheduler->entries[slot];
        if (entry->occupied) {
            game_sim_context_shutdown(out_context);
            game_scheduler_destroy(out_scheduler);
            game_world_map_destroy(out_world_map);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        uint8_t payload[GAME_SCHEDULER_MAX_PAYLOAD_BYTES] = {0};
        if (!reader_read_bytes(&reader, payload, GAME_SCHEDULER_MAX_PAYLOAD_BYTES)) {
            game_sim_context_shutdown(out_context);
            game_scheduler_destroy(out_scheduler);
            game_world_map_destroy(out_world_map);
            return GAME_SNAPSHOT_RESULT_MALFORMED;
        }

        *entry = (GameSchedulerEntry){
            .occupied = true,
            .cancelled = false,
            .fired = false,
            .generation = generation,
            .due_tick = due_tick,
            .sequence = sequence,
            .type = type,
            .payload_size = payload_size,
            .reserved = reserved,
            .source = {.index = source_index, .generation = source_generation},
        };
        if (payload_size > 0u) {
            memcpy(entry->payload, payload, payload_size);
        }

        ++out_scheduler->active_count;
    }

    return GAME_SNAPSHOT_RESULT_OK;
}
