#include "game/default_scene.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio/audio_request.h"
#include "colony/construction.h"
#include "colony/emergency.h"
#include "colony/haul_job.h"
#include "colony/inventory.h"
#include "core/log.h"
#include "nav/path_service.h"
#include "render/particle_request.h"
#include "sim/combat.h"
#include "sim/horde_materialization.h"
#include "sim/noise_system.h"
#include "sim/projectile.h"
#include "sim/replay.h"
#include "sim/sensory_fields.h"
#include "sim/status_effect.h"
#include "ui/causal_report.h"
#include "ui/command_inspector.h"
#include "world/structure.h"

#define GAME_DEFAULT_SCENE_WIDTH 180
#define GAME_DEFAULT_SCENE_HEIGHT 120
#define GAME_DEFAULT_SCENE_FIELD_PADDING 3

static void game_default_scene_zero_runtime(GameDefaultScene *scene)
{
    scene->seed = 0u;
    scene->tick = 0u;
    scene->party_actor = (GameEntityId){0u, 0u};
    scene->party_position = (GameHexAxial){0, 0};
    scene->causal_report_size = 0u;
    scene->causal_report[0] = '\0';
    scene->interaction_summary[0] = '\0';
    scene->last_interaction_kind = GAME_DEFAULT_SCENE_INTERACTION_NONE;
    scene->path_summary[0] = '\0';
    scene->command_history_count = 0u;
    scene->alert_count = 0u;
    scene->last_attack_result = GAME_DEFAULT_SCENE_ATTACK_NONE;
    scene->attack_summary[0] = '\0';
    scene->demo_status = (GameDefaultSceneDemoStatus){0};
}

static void game_default_scene_push_text(char entries[][96], size_t *count, size_t capacity, const char *text)
{
    if (!entries || !count || capacity == 0u || !text) {
        return;
    }
    if (*count < capacity) {
        (void)snprintf(entries[*count], 96u, "%s", text);
        *count += 1u;
        return;
    }
    for (size_t i = 1u; i < capacity; ++i) {
        memcpy(entries[i - 1u], entries[i], 96u);
    }
    (void)snprintf(entries[capacity - 1u], 96u, "%s", text);
}

static void game_default_scene_push_command(GameDefaultScene *scene, const char *text)
{
    if (scene) {
        game_default_scene_push_text(scene->command_history, &scene->command_history_count, 8u, text);
    }
}

static void game_default_scene_push_alert(GameDefaultScene *scene, const char *text)
{
    if (scene) {
        game_default_scene_push_text(scene->alerts, &scene->alert_count, 6u, text);
    }
}

static void game_default_scene_destroy_pathing(GameDefaultScene *scene)
{
    if (!scene) {
        return;
    }
    free(scene->path_cost_values);
    free(scene->path_g_score);
    free(scene->path_f_score);
    free(scene->path_parent);
    free(scene->path_open);
    free(scene->path_closed);
    free(scene->path_result_buffer);
    scene->path_cost_values = NULL;
    scene->path_g_score = NULL;
    scene->path_f_score = NULL;
    scene->path_parent = NULL;
    scene->path_open = NULL;
    scene->path_closed = NULL;
    scene->path_result_buffer = NULL;
    scene->path_result_capacity = 0u;
    scene->path_preview_length = 0u;
    scene->has_path_preview = false;
    scene->move_path_length = 0u;
    scene->move_path_cursor = 0u;
    scene->party_following_path = false;
}

static void game_default_scene_cleanup_partial(GameDefaultScene *scene)
{
    game_default_scene_destroy_pathing(scene);
    game_field_registry_destroy(&scene->sensory_fields);
    game_tile_field_destroy(&scene->noise_field);
    game_event_log_destroy(&scene->event_log);
    game_event_queue_destroy(&scene->event_queue);
    game_command_queue_destroy(&scene->command_queue);
    game_entity_registry_destroy(&scene->entities);
    game_scenario_destroy(&scene->scenario);
}

static GameDefaultSceneResult game_default_scene_init_pathing(GameDefaultScene *scene)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    GamePathGrid grid = {
        .q_min = 0,
        .r_min = 0,
        .q_count = (size_t)scene->scenario.horde_anchor.q + 1u,
        .r_count = (size_t)scene->scenario.horde_anchor.r + 1u,
    };
    size_t capacity = game_pathfind_required_capacity(&grid);
    if (capacity == 0u) {
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    scene->path_cost_values = (uint16_t *)malloc(sizeof(uint16_t) * capacity);
    scene->path_g_score = (uint32_t *)malloc(sizeof(uint32_t) * capacity);
    scene->path_f_score = (uint32_t *)malloc(sizeof(uint32_t) * capacity);
    scene->path_parent = (int32_t *)malloc(sizeof(int32_t) * capacity);
    scene->path_open = (bool *)malloc(sizeof(bool) * capacity);
    scene->path_closed = (bool *)malloc(sizeof(bool) * capacity);
    scene->path_result_capacity = GAME_DEFAULT_SCENE_MAX_PATH_TILES;
    scene->path_result_buffer = (GameHexAxial *)malloc(sizeof(GameHexAxial) * scene->path_result_capacity);
    if (!scene->path_cost_values || !scene->path_g_score || !scene->path_f_score || !scene->path_parent ||
        !scene->path_open || !scene->path_closed || !scene->path_result_buffer) {
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    scene->path_cost_map = (GamePathCostMap){
        .grid = grid,
        .cost = scene->path_cost_values,
        .blocked_cost = UINT16_MAX,
    };
    scene->path_scratch = (GamePathQueryScratch){
        .capacity = capacity,
        .g_score = scene->path_g_score,
        .f_score = scene->path_f_score,
        .parent = scene->path_parent,
        .open = scene->path_open,
        .closed = scene->path_closed,
    };

    for (int32_t q = 0; q <= scene->scenario.horde_anchor.q; ++q) {
        for (int32_t r = 0; r <= scene->scenario.horde_anchor.r; ++r) {
            GameHexAxial tile = {q, r};
            int32_t terrain = -1;
            size_t index = game_pathfind_index(&scene->path_cost_map, tile);
            if (index == (size_t)-1) {
                return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
            }
            if (game_world_map_get(&scene->scenario.map, tile, &terrain) != GAME_WORLD_MAP_RESULT_OK || terrain < 0) {
                scene->path_cost_values[index] = UINT16_MAX;
            } else {
                scene->path_cost_values[index] = 1u;
            }
        }
    }

    if (game_path_service_init(&scene->path_service, scene->path_slots, 4u, 2u) != GAME_PATH_SERVICE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    scene->path_summary[0] = '\0';
    scene->has_path_preview = false;
    scene->party_following_path = false;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static GameDefaultSceneResult game_default_scene_init_sensory_fields(GameDefaultScene *scene)
{
    if (game_field_registry_init(&scene->sensory_fields, scene->sensory_field_slots, 3u) !=
        GAME_FIELD_REGISTRY_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    GameTileFieldConfig field_config = {
        .q_min = -GAME_DEFAULT_SCENE_FIELD_PADDING,
        .r_min = -GAME_DEFAULT_SCENE_FIELD_PADDING,
        .q_count = (size_t)scene->scenario.horde_anchor.q + (size_t)GAME_DEFAULT_SCENE_FIELD_PADDING + 1u,
        .r_count = (size_t)scene->scenario.horde_anchor.r + (size_t)GAME_DEFAULT_SCENE_FIELD_PADDING + 1u,
        .min_value = 0,
        .max_value = 100,
    };
    if (game_field_registry_define_field(&scene->sensory_fields, GAME_FIELD_ID_LIGHT, &field_config) !=
            GAME_FIELD_REGISTRY_RESULT_OK ||
        game_field_registry_define_field(&scene->sensory_fields, GAME_FIELD_ID_SCENT, &field_config) !=
            GAME_FIELD_REGISTRY_RESULT_OK ||
        game_field_registry_define_field(&scene->sensory_fields, GAME_FIELD_ID_BLOOD, &field_config) !=
            GAME_FIELD_REGISTRY_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_init(GameDefaultScene *scene)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    *scene = (GameDefaultScene){0};
    scene->seed = 1337u;

    GameScenarioConfig scenario_config = {
        .seed = scene->seed,
        .width = GAME_DEFAULT_SCENE_WIDTH,
        .height = GAME_DEFAULT_SCENE_HEIGHT,
        .block_chance_percent = 16u,
        .include_stockpile = true,
    };
    if (game_scenario_generate(&scene->scenario, &scenario_config) != GAME_SCENARIO_GEN_RESULT_OK) {
        game_default_scene_cleanup_partial(scene);
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }
    scene->party_position = scene->scenario.party_anchor;
    if (game_default_scene_init_pathing(scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        game_default_scene_cleanup_partial(scene);
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    if (game_entity_registry_init(&scene->entities, 16u) != GAME_ENTITY_RESULT_OK ||
        game_entity_registry_create(&scene->entities, &scene->party_actor) != GAME_ENTITY_RESULT_OK) {
        game_default_scene_cleanup_partial(scene);
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    GameCommandQueueConfig command_config = {
        .capacity = 8u,
        .accepted_tick_window = 8u,
    };
    if (game_command_queue_init(&scene->command_queue, &command_config) != GAME_COMMAND_QUEUE_RESULT_OK ||
        game_event_queue_init(&scene->event_queue, 64u) != GAME_EVENT_QUEUE_RESULT_OK ||
        game_event_log_init(&scene->event_log, 2048u) != GAME_EVENT_LOG_RESULT_OK) {
        game_default_scene_cleanup_partial(scene);
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    GameTileFieldConfig noise_config = {
        .q_min = -GAME_DEFAULT_SCENE_FIELD_PADDING,
        .r_min = -GAME_DEFAULT_SCENE_FIELD_PADDING,
        .q_count = (size_t)scene->scenario.horde_anchor.q + (size_t)GAME_DEFAULT_SCENE_FIELD_PADDING + 1u,
        .r_count = (size_t)scene->scenario.horde_anchor.r + (size_t)GAME_DEFAULT_SCENE_FIELD_PADDING + 1u,
        .min_value = 0,
        .max_value = 100,
    };
    if (game_tile_field_init(&scene->noise_field, &noise_config) != GAME_TILE_FIELD_RESULT_OK) {
        game_default_scene_cleanup_partial(scene);
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }
    if (game_default_scene_init_sensory_fields(scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        game_default_scene_cleanup_partial(scene);
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }

    GameHordeAttentionConfig attention_config = {
        .pressurize_per_sample = 1u,
        .decay_per_tick = 1u,
        .attack_threshold = 10u,
        .calm_threshold = 4u,
        .max_pressure = 100u,
    };
    game_horde_attention_init(&scene->horde_attention, &attention_config);

    if (game_job_board_init(&scene->job_board, scene->job_slots, 4u, 6u) != GAME_JOB_BOARD_RESULT_OK) {
        game_default_scene_cleanup_partial(scene);
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }
    game_worker_ai_audit_init(&scene->worker_audit_log, scene->worker_audits, 4u);
    scene->worker = (GameWorkerState){
        .id = (GameEntityId){2u, 0u},
        .role_flags = GAME_WORKER_ROLE_STANDARD,
        .stamina = 100u,
        .position = {scene->scenario.colony_anchor.q, scene->scenario.colony_anchor.r > 0
                                                          ? scene->scenario.colony_anchor.r - 1
                                                          : scene->scenario.colony_anchor.r},
        .threat_level = 0u,
    };
    if (game_horde_group_init(&scene->demo_horde_group, 1u, 1u) != GAME_HORDE_GROUP_RESULT_OK) {
        game_default_scene_cleanup_partial(scene);
        return GAME_DEFAULT_SCENE_RESULT_INIT_FAILED;
    }
    game_horde_materialization_init(&scene->demo_materialization, scene->demo_horde_group.id, scene->demo_horde_actors,
                                    4u);

    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static bool game_default_scene_tile_is_open(const GameDefaultScene *scene, GameHexAxial tile)
{
    if (!scene) {
        return false;
    }

    int32_t terrain = -1;
    return game_world_map_get(&scene->scenario.map, tile, &terrain) == GAME_WORLD_MAP_RESULT_OK && terrain >= 0;
}

static GameDefaultSceneResult game_default_scene_stage_tile_command(GameDefaultScene *scene, GameCommandType type,
                                                                    GameHexAxial target,
                                                                    GameCommandQueueResult *out_command_result)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    GameCommandTilePayload payload = {
        .target_q = target.q,
        .target_r = target.r,
        .flags = 0u,
    };
    GameCommand command = {
        .type = type,
        .requested_tick = scene->tick,
        .source = scene->party_actor,
        .payload_size = sizeof(payload),
    };
    memcpy(command.payload.bytes, &payload, sizeof(payload));

    GameCommandQueueResult command_result =
        game_command_queue_push(&scene->command_queue, &command, scene->tick, &scene->entities);
    if (out_command_result) {
        *out_command_result = command_result;
    }
    return command_result == GAME_COMMAND_QUEUE_RESULT_OK ? GAME_DEFAULT_SCENE_RESULT_OK
                                                          : GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
}

static GameDefaultSceneResult game_default_scene_pop_tile_command(GameDefaultScene *scene,
                                                                  GameCommandType expected_type,
                                                                  GameCommandTilePayload *out_payload)
{
    if (!scene || !out_payload) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    GameCommand command = {0};
    if (game_command_queue_pop(&scene->command_queue, &command) != GAME_COMMAND_QUEUE_RESULT_OK ||
        command.type != expected_type || command.payload_size != sizeof(*out_payload)) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    memcpy(out_payload, command.payload.bytes, sizeof(*out_payload));
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

void game_default_scene_shutdown(GameDefaultScene *scene)
{
    if (!scene) {
        return;
    }
    game_default_scene_cleanup_partial(scene);
    game_default_scene_zero_runtime(scene);
}

static GameDefaultSceneResult game_default_scene_stage_party_noise(GameDefaultScene *scene, GameHexAxial origin,
                                                                   GameCommandQueueResult *out_command_result)
{
    GameCommandNoisePayload payload = {
        .origin_q = origin.q,
        .origin_r = origin.r,
        .intensity = 14,
        .max_radius = 3u,
        .attenuation = 2u,
        .decay_ticks = 6u,
    };

    GameCommandInspector inspector = {0};
    game_command_inspector_init(&inspector);
    GameCommandQueueResult command_result =
        game_command_inspector_stage_noise(&inspector, scene->party_actor, scene->tick, &payload);
    if (command_result == GAME_COMMAND_QUEUE_RESULT_OK) {
        command_result =
            game_command_inspector_submit(&inspector, &scene->command_queue, scene->tick, &scene->entities);
    }

    if (out_command_result) {
        *out_command_result = command_result;
    }
    if (command_result != GAME_COMMAND_QUEUE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static GameDefaultSceneResult game_default_scene_apply_noise(GameDefaultScene *scene, uint64_t *out_field_sequence)
{
    GameCommand command = {0};
    if (game_command_queue_pop(&scene->command_queue, &command) != GAME_COMMAND_QUEUE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameNoiseAppliedResult applied = {0};
    if (game_noise_emit_from_command(&command, scene->tick, &scene->event_queue, &scene->event_log, &scene->noise_field,
                                     &applied) != GAME_NOISE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    if (!applied.field_event_appended) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    *out_field_sequence = applied.field_event_seq;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static GameDefaultSceneResult game_default_scene_apply_horde_reaction(GameDefaultScene *scene, GameHexAxial sample_tile,
                                                                      uint64_t parent_sequence)
{
    int32_t sample = 0;
    if (game_tile_field_get(&scene->noise_field, sample_tile, &sample) != GAME_TILE_FIELD_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameHordeAttentionUpdateResult update = {0};
    if (game_horde_attention_tick(&scene->horde_attention, (uint32_t)sample, scene->party_actor, scene->tick,
                                  parent_sequence, &scene->event_log, &update) != GAME_HORDE_ATTENTION_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    if (!update.emitted) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    return game_causal_report_write(&scene->event_log, update.emitted_event_seq, scene->causal_report,
                                    sizeof(scene->causal_report),
                                    &scene->causal_report_size) == GAME_CAUSAL_REPORT_RESULT_OK
               ? GAME_DEFAULT_SCENE_RESULT_OK
               : GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
}

static GameDefaultSceneResult game_default_scene_apply_colony_autonomy(GameDefaultScene *scene)
{
    GameJobOrderHandle stockpile_job = {0};
    if (game_job_board_create_order(&scene->job_board, scene->tick, scene->scenario.stockpile_anchor,
                                    GAME_WORKER_ROLE_STANDARD, 0u, 3u, 6u,
                                    &stockpile_job) != GAME_JOB_BOARD_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameWorkerSelectionConfig worker_config = {
        .current_tick = scene->tick,
        .threat_stall_threshold = 3u,
        .emergency_role_mask = GAME_WORKER_ROLE_EMERGENCY,
    };
    GameWorkerSelectionResultData worker_result = {0};
    if (game_worker_ai_select_job_for_worker(&scene->worker, &scene->job_board, &worker_config,
                                             &scene->worker_audit_log, &worker_result) != GAME_WORKER_AI_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    return worker_result.selected_any ? GAME_DEFAULT_SCENE_RESULT_OK : GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
}

GameDefaultSceneResult game_default_scene_emit_noise_at(GameDefaultScene *scene, GameHexAxial origin,
                                                        GameCommandQueueResult *out_command_result)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    uint64_t field_sequence = 0u;
    GameDefaultSceneResult result = game_default_scene_stage_party_noise(scene, origin, out_command_result);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        return result;
    }
    result = game_default_scene_apply_noise(scene, &field_sequence);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        return result;
    }
    result = game_default_scene_apply_horde_reaction(scene, origin, field_sequence);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        return result;
    }

    scene->tick += 1u;
    char history[96] = {0};
    (void)snprintf(history, sizeof(history), "Noise q%d r%d pressure %u", origin.q, origin.r,
                   scene->horde_attention.pressure);
    game_default_scene_push_command(scene, history);
    game_default_scene_push_alert(scene, "Noise raised horde attention.");
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static GameDefaultSceneResult game_default_scene_compute_path(GameDefaultScene *scene, GameHexAxial start,
                                                              GameHexAxial target, GameHexAxial *out_path,
                                                              size_t out_capacity, size_t *out_length)
{
    if (!scene || !out_path || !out_length || out_capacity == 0u) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    *out_length = 0u;
    if (!game_default_scene_tile_is_open(scene, start) || !game_default_scene_tile_is_open(scene, target)) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    (void)game_path_service_advance_tick(&scene->path_service, scene->tick + 4u, UINT32_MAX);
    GamePathRequestHandle handle = {0};
    if (game_path_service_submit(&scene->path_service, scene->tick, start, target, &scene->path_cost_map,
                                 &scene->path_scratch, 1u, scene->path_result_buffer, scene->path_result_capacity,
                                 &handle) != GAME_PATH_SERVICE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    if (game_path_service_advance_tick(&scene->path_service, scene->tick, UINT32_MAX) != GAME_PATH_SERVICE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GamePathServiceStatus status = {0};
    if (game_path_service_status(&scene->path_service, handle, &status) != GAME_PATH_SERVICE_RESULT_OK ||
        status.state != GAME_PATH_SERVICE_STATE_RESOLVED || !status.has_result_path) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    uint64_t result_version = 0u;
    if (game_path_service_retrieve(&scene->path_service, handle, out_path, out_capacity, out_length, &result_version) !=
        GAME_PATH_SERVICE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_preview_path_to(GameDefaultScene *scene, GameHexAxial target)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    scene->has_path_preview = false;
    scene->path_preview_length = 0u;
    scene->path_preview_goal = target;
    if (!game_default_scene_tile_is_open(scene, target)) {
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "No route: q%d r%d is blocked", target.q,
                       target.r);
        game_default_scene_push_alert(scene, "Move preview rejected: blocked target.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    size_t length = 0u;
    GameDefaultSceneResult result = game_default_scene_compute_path(
        scene, scene->party_position, target, scene->path_preview, GAME_DEFAULT_SCENE_MAX_PATH_TILES, &length);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "No route to q%d r%d", target.q, target.r);
        game_default_scene_push_alert(scene, "Move preview failed: no route.");
        return result;
    }

    scene->path_preview_length = length;
    scene->has_path_preview = true;
    (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "Path preview: %zu tiles to q%d r%d", length,
                   target.q, target.r);
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_update(GameDefaultScene *scene)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }
    if (!scene->party_following_path) {
        return GAME_DEFAULT_SCENE_RESULT_OK;
    }
    if (scene->move_path_cursor >= scene->move_path_length) {
        scene->party_following_path = false;
        scene->has_path_preview = false;
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "Arrived at q%d r%d", scene->party_position.q,
                       scene->party_position.r);
        return GAME_DEFAULT_SCENE_RESULT_OK;
    }

    GameHexAxial next = scene->move_path[scene->move_path_cursor++];
    if (!game_default_scene_tile_is_open(scene, next)) {
        scene->party_following_path = false;
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "Path blocked at q%d r%d", next.q, next.r);
        game_default_scene_push_alert(scene, "Move interrupted by blocked path.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    scene->party_position = next;
    uint64_t move_sequence = 0u;
    if (game_event_log_append(&scene->event_log, scene->tick, GAME_EVENT_TYPE_PARTY_MOVED, scene->party_actor,
                              GAME_EVENT_LOG_INVALID_PARENT_ID, &move_sequence) != GAME_EVENT_LOG_RESULT_OK) {
        scene->party_following_path = false;
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    if (scene->move_path_cursor >= scene->move_path_length) {
        scene->party_following_path = false;
        scene->has_path_preview = false;
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "Arrived at q%d r%d", scene->party_position.q,
                       scene->party_position.r);
        game_default_scene_push_alert(scene, "Party arrived at move target.");
    } else {
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "Moving: step %zu/%zu toward q%d r%d",
                       scene->move_path_cursor, scene->move_path_length - 1u, scene->move_goal.q, scene->move_goal.r);
    }
    scene->tick += 1u;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_move_party_to(GameDefaultScene *scene, GameHexAxial target,
                                                        GameCommandQueueResult *out_command_result)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }
    if (!game_default_scene_tile_is_open(scene, target)) {
        if (out_command_result) {
            *out_command_result = GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD;
        }
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary), "Move blocked at q%d r%d",
                       target.q, target.r);
        game_default_scene_push_alert(scene, "Move order rejected: blocked target.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameDefaultSceneResult result =
        game_default_scene_stage_tile_command(scene, GAME_COMMAND_TYPE_MOVE_PARTY, target, out_command_result);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        return result;
    }

    GameCommandTilePayload payload = {0};
    if (game_default_scene_pop_tile_command(scene, GAME_COMMAND_TYPE_MOVE_PARTY, &payload) !=
        GAME_DEFAULT_SCENE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameHexAxial goal = {payload.target_q, payload.target_r};
    size_t length = 0u;
    if (game_default_scene_compute_path(scene, scene->party_position, goal, scene->move_path,
                                        GAME_DEFAULT_SCENE_MAX_PATH_TILES, &length) != GAME_DEFAULT_SCENE_RESULT_OK) {
        if (out_command_result) {
            *out_command_result = GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD;
        }
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "No route to q%d r%d", goal.q, goal.r);
        game_default_scene_push_alert(scene, "Move order rejected: no route.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    scene->move_path_length = length;
    scene->move_path_cursor = length > 1u ? 1u : 0u;
    scene->move_goal = goal;
    scene->party_following_path = length > 1u;
    memcpy(scene->path_preview, scene->move_path, sizeof(GameHexAxial) * length);
    scene->path_preview_length = length;
    scene->path_preview_goal = goal;
    scene->has_path_preview = length > 1u;

    if (length <= 1u) {
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "Already at q%d r%d", scene->party_position.q,
                       scene->party_position.r);
    } else {
        (void)snprintf(scene->path_summary, sizeof(scene->path_summary), "Move order: %zu steps to q%d r%d",
                       length - 1u, goal.q, goal.r);
    }
    (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary), "Move order set for q%d r%d", goal.q,
                   goal.r);
    char history[96] = {0};
    (void)snprintf(history, sizeof(history), "Move q%d r%d steps %zu", goal.q, goal.r, length > 0u ? length - 1u : 0u);
    game_default_scene_push_command(scene, history);
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_interact_at(GameDefaultScene *scene, GameHexAxial target,
                                                      GameCommandQueueResult *out_command_result)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }
    if (!game_default_scene_tile_is_open(scene, target)) {
        if (out_command_result) {
            *out_command_result = GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD;
        }
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary), "No access at q%d r%d", target.q,
                       target.r);
        game_default_scene_push_alert(scene, "Interact rejected: inaccessible target.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameDefaultSceneResult result =
        game_default_scene_stage_tile_command(scene, GAME_COMMAND_TYPE_INTERACT_WORLD, target, out_command_result);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        return result;
    }

    GameCommandTilePayload payload = {0};
    if (game_default_scene_pop_tile_command(scene, GAME_COMMAND_TYPE_INTERACT_WORLD, &payload) !=
        GAME_DEFAULT_SCENE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameHexAxial interacted = {payload.target_q, payload.target_r};
    int32_t noise = 0;
    (void)game_tile_field_get(&scene->noise_field, interacted, &noise);
    if (interacted.q == scene->party_position.q && interacted.r == scene->party_position.r) {
        scene->last_interaction_kind = GAME_DEFAULT_SCENE_INTERACTION_PARTY;
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary), "Party holding at q%d r%d",
                       interacted.q, interacted.r);
    } else if (interacted.q == scene->worker.position.q && interacted.r == scene->worker.position.r) {
        scene->last_interaction_kind = GAME_DEFAULT_SCENE_INTERACTION_WORKER;
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary), "Worker stamina %u job=%s",
                       scene->worker.stamina, scene->worker.has_current_job ? "active" : "idle");
    } else if (interacted.q == scene->scenario.colony_anchor.q && interacted.r == scene->scenario.colony_anchor.r) {
        scene->last_interaction_kind = GAME_DEFAULT_SCENE_INTERACTION_COLONY;
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary),
                       "Colony hub: audits %zu pressure %u stockpile=%s", scene->worker_audit_log.count,
                       scene->horde_attention.pressure, scene->scenario.has_stockpile ? "yes" : "no");
    } else if (scene->scenario.has_stockpile && interacted.q == scene->scenario.stockpile_anchor.q &&
               interacted.r == scene->scenario.stockpile_anchor.r) {
        scene->last_interaction_kind = GAME_DEFAULT_SCENE_INTERACTION_STOCKPILE;
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary), "Stockpile anchor at q%d r%d",
                       interacted.q, interacted.r);
    } else if ((interacted.q == scene->scenario.horde_anchor.q && interacted.r == scene->scenario.horde_anchor.r) ||
               (scene->demo_status.horde_materialized && interacted.q == scene->demo_status.horde_spawn_tile.q &&
                interacted.r == scene->demo_status.horde_spawn_tile.r)) {
        scene->last_interaction_kind = GAME_DEFAULT_SCENE_INTERACTION_HORDE;
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary),
                       "Horde contact: pressure %u mass %u", scene->horde_attention.pressure,
                       scene->demo_horde_group.mass_estimate);
    } else if (noise > 0) {
        scene->last_interaction_kind = GAME_DEFAULT_SCENE_INTERACTION_NOISE;
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary),
                       "Noise trace at q%d r%d value %d", interacted.q, interacted.r, noise);
    } else {
        scene->last_interaction_kind = GAME_DEFAULT_SCENE_INTERACTION_TERRAIN;
        (void)snprintf(scene->interaction_summary, sizeof(scene->interaction_summary), "Terrain probe q%d r%d: open",
                       interacted.q, interacted.r);
    }

    uint64_t interact_sequence = 0u;
    if (game_event_log_append(&scene->event_log, scene->tick, GAME_EVENT_TYPE_WORLD_INTERACTED, scene->party_actor,
                              GAME_EVENT_LOG_INVALID_PARENT_ID, &interact_sequence) != GAME_EVENT_LOG_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    scene->tick += 1u;
    char history[96] = {0};
    (void)snprintf(history, sizeof(history), "Interact q%d r%d kind %d", interacted.q, interacted.r,
                   (int)scene->last_interaction_kind);
    game_default_scene_push_command(scene, history);
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_attack_at(GameDefaultScene *scene, GameHexAxial target,
                                                    GameCommandQueueResult *out_command_result)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    bool horde_target = (target.q == scene->scenario.horde_anchor.q && target.r == scene->scenario.horde_anchor.r) ||
                        (scene->demo_status.horde_materialized && target.q == scene->demo_status.horde_spawn_tile.q &&
                         target.r == scene->demo_status.horde_spawn_tile.r);
    GameEntityId target_entity = game_entity_invalid_id();
    if (scene->demo_status.horde_materialized && scene->demo_materialization.actor_count > 0u) {
        for (size_t i = 0u; i < scene->demo_materialization.actor_count; ++i) {
            GameEntityId candidate = scene->demo_materialization.actor_ids[i];
            if (game_entity_id_is_valid(candidate) && game_entity_registry_is_alive(&scene->entities, candidate)) {
                target_entity = candidate;
                break;
            }
        }
    }
    if (!horde_target || !game_entity_id_is_valid(target_entity)) {
        if (out_command_result) {
            *out_command_result = GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD;
        }
        scene->last_attack_result = GAME_DEFAULT_SCENE_ATTACK_INVALID_TARGET;
        (void)snprintf(scene->attack_summary, sizeof(scene->attack_summary), "No hostile target at q%d r%d", target.q,
                       target.r);
        game_default_scene_push_alert(scene, "Attack rejected: no hostile target.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameDefaultSceneResult result =
        game_default_scene_stage_tile_command(scene, GAME_COMMAND_TYPE_ATTACK_TARGET, target, out_command_result);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        return result;
    }

    GameCommandTilePayload payload = {0};
    if (game_default_scene_pop_tile_command(scene, GAME_COMMAND_TYPE_ATTACK_TARGET, &payload) !=
        GAME_DEFAULT_SCENE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameCombatAbility abilities[1] = {0};
    GameCombatAbilityCatalog catalog = {0};
    GameCombatCooldownSlot cooldown_slots[1] = {0};
    GameCombatCooldownTable cooldowns = {0};
    GameCombatResultData combat = {0};
    if (game_combat_init_catalog(&catalog, abilities, 1u) != GAME_COMBAT_RESULT_OK ||
        game_combat_catalog_set(&catalog, 10u, 320u, 4u, 6) != GAME_COMBAT_RESULT_OK ||
        game_combat_cooldown_init(&cooldowns, cooldown_slots, 1u) != GAME_COMBAT_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    GameCombatAbilityCommand attack = {
        .ability_id = 10u,
        .actor = scene->party_actor,
        .target = target_entity,
        .actor_position = scene->party_position,
        .target_position = {payload.target_q, payload.target_r},
    };
    if (game_combat_resolve_ability(&catalog, &cooldowns, &attack, scene->tick, &scene->event_queue, &scene->event_log,
                                    &combat) != GAME_COMBAT_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    scene->last_attack_result = GAME_DEFAULT_SCENE_ATTACK_HIT;
    (void)snprintf(scene->attack_summary, sizeof(scene->attack_summary), "Attack hit q%d r%d damage %d",
                   payload.target_q, payload.target_r, combat.damage);
    scene->horde_attention.pressure =
        scene->horde_attention.pressure + 2u > 100u ? 100u : scene->horde_attention.pressure + 2u;
    char history[96] = {0};
    (void)snprintf(history, sizeof(history), "Attack q%d r%d damage %d", payload.target_q, payload.target_r,
                   combat.damage);
    game_default_scene_push_command(scene, history);
    game_default_scene_push_alert(scene, "Attack resolved against horde contact.");
    scene->tick += 1u;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static uint64_t game_default_scene_latest_event_sequence(const GameDefaultScene *scene)
{
    size_t count = game_event_log_count(&scene->event_log);
    const GameEventLogEntry *entry = count > 0u ? game_event_log_at(&scene->event_log, count - 1u) : NULL;
    return entry ? entry->sequence : GAME_EVENT_LOG_INVALID_PARENT_ID;
}

static GameDefaultSceneResult game_default_scene_run_demo_fields(GameDefaultScene *scene, uint64_t parent_sequence)
{
    GameSensoryFieldImpulseResult light_result = {0};
    GameSensoryFieldImpulseResult scent_result = {0};
    GameSensoryFieldImpulseResult blood_result = {0};
    if (game_sensory_apply_light_impulse(&scene->sensory_fields, scene->scenario.party_anchor, 18, 3, 2, scene->tick,
                                         parent_sequence, &scene->event_log,
                                         &light_result) != GAME_SENSORY_FIELDS_RESULT_OK ||
        game_sensory_apply_scent_impulse(&scene->sensory_fields, scene->scenario.colony_anchor, 13, 3, 1, scene->tick,
                                         parent_sequence, &scene->event_log,
                                         &scent_result) != GAME_SENSORY_FIELDS_RESULT_OK ||
        game_sensory_apply_blood_impulse(&scene->sensory_fields, scene->scenario.horde_anchor, 9, 2, 2, scene->tick,
                                         parent_sequence, &scene->event_log,
                                         &blood_result) != GAME_SENSORY_FIELDS_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    int32_t light = 0;
    int32_t scent = 0;
    int32_t blood = 0;
    if (game_sensory_sample_field(&scene->sensory_fields, GAME_FIELD_ID_LIGHT, scene->scenario.party_anchor, &light) !=
            GAME_SENSORY_FIELDS_RESULT_OK ||
        game_sensory_sample_field(&scene->sensory_fields, GAME_FIELD_ID_SCENT, scene->scenario.colony_anchor, &scent) !=
            GAME_SENSORY_FIELDS_RESULT_OK ||
        game_sensory_sample_field(&scene->sensory_fields, GAME_FIELD_ID_BLOOD, scene->scenario.horde_anchor, &blood) !=
            GAME_SENSORY_FIELDS_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    scene->demo_status.light_sample = (uint32_t)light;
    scene->demo_status.scent_sample = (uint32_t)scent;
    scene->demo_status.blood_sample = (uint32_t)blood;
    (void)snprintf(scene->demo_status.field_summary, sizeof(scene->demo_status.field_summary),
                   "Fields light=%d scent=%d blood=%d", light, scent, blood);
    scene->demo_status.stages_completed++;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static GameDefaultSceneResult game_default_scene_run_demo_horde(GameDefaultScene *scene, uint64_t parent_sequence)
{
    GameHordeGroupConfig group_config = {
        .pressure_gain_per_sample = 1u,
        .decay_per_tick = 1u,
        .threatened_threshold = 12u,
        .calm_threshold = 4u,
        .migration_threshold = 24u,
        .migration_transfer = 4u,
    };
    GameWorldTopologyRegionId neighbor_regions[2] = {2u, 3u};
    uint32_t neighbor_samples[2] = {3u, 6u};
    GameHordeGroupUpdateResult group_update = {0};
    if (game_horde_group_tick(&scene->demo_horde_group, &group_config, 18u, scene->party_actor, neighbor_regions,
                              neighbor_samples, 2u, scene->tick, parent_sequence, &scene->event_log,
                              &group_update) != GAME_HORDE_GROUP_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameHordeMaterializationConfig materialization_config = {
        .materialize_pressure_threshold = 12u,
        .dematerialize_pressure_threshold = 4u,
        .pressure_cost_per_actor = 4u,
        .pressure_return_per_actor = 2u,
        .spawn_budget_per_tick = 2u,
        .dematerialize_budget_per_tick = 2u,
        .mass_per_actor = 1u,
        .max_spawn_proximity = 5u,
        .spawn_actor_count = 2u,
    };
    GameHordeMaterializationUpdateResult materialized = {0};
    if (game_horde_materialization_apply(
            &scene->demo_materialization, &scene->demo_horde_group, &materialization_config, true, 1u, scene->tick,
            group_update.emitted ? group_update.emitted_event_sequence : parent_sequence, &scene->entities,
            &scene->event_queue, &scene->event_log, &materialized) != GAME_HORDE_MATERIALIZATION_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    scene->demo_status.horde_materialized = materialized.spawned;
    scene->demo_status.horde_spawned_count = materialized.spawned_count;
    scene->demo_status.horde_group_pressure = scene->demo_horde_group.pressure;
    scene->demo_status.horde_group_mass = scene->demo_horde_group.mass_estimate;
    scene->demo_status.horde_spawn_tile = scene->scenario.horde_anchor;
    (void)snprintf(scene->demo_status.horde_summary, sizeof(scene->demo_status.horde_summary),
                   "Horde p=%u mass=%u spawn=%u", scene->demo_status.horde_group_pressure,
                   scene->demo_status.horde_group_mass, scene->demo_status.horde_spawned_count);
    scene->demo_status.stages_completed++;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static GameDefaultSceneResult game_default_scene_run_demo_colony(GameDefaultScene *scene)
{
    GameInventoryEntry inventory_entries[8] = {0};
    GameInventoryReservation inventory_reservations[4] = {0};
    GameInventory inventory = {0};
    if (game_inventory_init(&inventory, inventory_entries, 8u, inventory_reservations, 4u, 16u) !=
        GAME_INVENTORY_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameInventoryOwner source_owner = {GAME_INVENTORY_OWNER_KIND_ENTITY, {401u, 1u}, 0u};
    GameInventoryOwner destination_owner = {GAME_INVENTORY_OWNER_KIND_STOCKPILE, {0u, 0u}, 1u};
    if (game_inventory_add(&inventory, source_owner, 77u, 2u, NULL) != GAME_INVENTORY_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameJobBoardOrderSlot board_slots[8] = {0};
    GameJobBoard board = {0};
    if (game_job_board_init(&board, board_slots, 8u, 4u) != GAME_JOB_BOARD_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    GameHaulJob haul_slots[4] = {0};
    GameHaulJobSystem haul_system = {0};
    if (game_haul_job_init(&haul_system, haul_slots, 4u) != GAME_HAUL_JOB_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameHaulJobCreateInfo haul_info = {
        .worker = scene->worker.id,
        .route_request = {0u, 0u},
        .source_owner = source_owner,
        .destination_owner = destination_owner,
        .source_tile = scene->scenario.stockpile_anchor,
        .destination_tile = scene->scenario.colony_anchor,
        .resource_id = 77u,
        .required_amount = 2u,
        .worker_capacity = 2u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
    };
    GameHaulJobHandle active_haul = {0u, 0u};
    if (game_haul_job_create(&haul_system, &inventory, &board, scene->tick, &haul_info, &active_haul) !=
        GAME_HAUL_JOB_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameJobOrderHandle routine_order = {0u, 0u};
    if (game_job_board_create_order(&board, scene->tick, scene->scenario.stockpile_anchor, GAME_WORKER_ROLE_STANDARD,
                                    0u, 2u, 4u, &routine_order) != GAME_JOB_BOARD_RESULT_OK ||
        game_job_board_reserve(&board, routine_order, scene->tick, scene->worker.id) != GAME_JOB_BOARD_RESULT_OK ||
        game_job_board_transition(&board, routine_order, GAME_JOB_BOARD_STATE_IN_PROGRESS) !=
            GAME_JOB_BOARD_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameWorkerState worker = scene->worker;
    worker.has_current_job = true;
    worker.current_job = routine_order;
    GameEmergencyInterruption interruptions[2] = {0};
    GameEmergencySystem emergency_system = {0};
    if (game_emergency_init(&emergency_system, interruptions, 2u) != GAME_EMERGENCY_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    GameEmergencyAuditEntry audit_entries[8] = {0};
    GameEmergencyAuditLog audit_log = {0};
    game_emergency_audit_init(&audit_log, audit_entries, 8u);
    GameEmergencyResult high = game_emergency_update(
        &emergency_system, &board, &inventory, &haul_system, &worker, active_haul, scene->tick + 1u,
        scene->horde_attention.pressure, 3u, true, scene->scenario.colony_anchor,
        GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY, 0u, 1u, 1u, &audit_log);
    GameEmergencyResult clear = game_emergency_update(
        &emergency_system, &board, &inventory, &haul_system, &worker, active_haul, scene->tick + 2u, 0u, 3u, true,
        scene->scenario.colony_anchor, GAME_WORKER_ROLE_STANDARD | GAME_WORKER_ROLE_EMERGENCY, 0u, 1u, 1u, &audit_log);
    if ((high != GAME_EMERGENCY_RESULT_OK && high != GAME_EMERGENCY_RESULT_ALREADY_ACTIVE) ||
        clear != GAME_EMERGENCY_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameConstructionWorksite construction_sites[2] = {0};
    GameConstructionSystem construction = {0};
    GameConstructionAuditEntry construction_audits[8] = {0};
    GameConstructionAuditLog construction_log = {0};
    static const GameStructureFootprintOffset footprint[1] = {{0, 0}};
    static const GameStructureDefinition structures[1] = {
        {101u, GAME_STRUCTURE_TILE_FLAG_BLOCKS_PATH, GAME_STRUCTURE_FIELD_FLAG_NONE, 1u, footprint},
    };
    GameStructurePlacement placements[2] = {0};
    GameStructureSystem structure_system = {0};
    GameConstructionResourceRequirement requirements[1] = {{77u, 1u, 0u}};
    GameConstructionWorksiteHandle worksite_handle = {0u, 0u};
    game_construction_audit_init(&construction_log, construction_audits, 8u);
    if (game_construction_init(&construction, construction_sites, 2u, 1u) != GAME_CONSTRUCTION_RESULT_OK ||
        game_structure_init(&structure_system, structures, 1u, placements, 2u, 1u) != GAME_STRUCTURE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    GameConstructionCreateInfo construction_info = {
        .target_tile = scene->scenario.colony_anchor,
        .structure_definition_id = 101u,
        .assigned_worker = scene->worker.id,
        .resource_source_owner = source_owner,
        .resource_destination_owner = destination_owner,
        .required_work_ticks = 2u,
        .worker_capacity = 1u,
        .required_role_flags = GAME_WORKER_ROLE_STANDARD,
        .required_resource_flags = 0u,
        .duration_min_ticks = 1u,
        .duration_max_ticks = 1u,
        .haul_route_request = {0u, 0u},
        .requirements = requirements,
        .requirement_count = 1u,
    };
    if (game_construction_create(&construction, &board, &inventory, scene->tick, &construction_info,
                                 &worksite_handle) != GAME_CONSTRUCTION_RESULT_OK ||
        game_construction_update(&construction, &board, &inventory, &haul_system, &structure_system,
                                 &scene->scenario.map, scene->tick + 3u, worksite_handle, &scene->event_log,
                                 &construction_log) != GAME_CONSTRUCTION_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    scene->demo_status.construction_started = construction_log.count > 0u;
    scene->demo_status.construction_progress = game_construction_progress(&construction, worksite_handle);

    scene->demo_status.emergency_audit_count = (uint32_t)audit_log.count;
    scene->demo_status.worker_interrupted = audit_log.count >= 1u;
    scene->demo_status.worker_resumed = worker.has_current_job;
    (void)snprintf(scene->demo_status.colony_summary, sizeof(scene->demo_status.colony_summary),
                   "Colony emerg=%u build=%s %u%%", scene->demo_status.emergency_audit_count,
                   scene->demo_status.construction_started ? "started" : "idle",
                   scene->demo_status.construction_progress);
    scene->demo_status.stages_completed++;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static bool game_default_scene_project_event_requests(const GameEvent *event, uint64_t sequence, uint64_t tick,
                                                      GameAudioRequestQueue *audio_queue,
                                                      GameParticleRequestQueue *particle_queue)
{
    bool projected = false;
    if (game_audio_request_project_from_event(event, tick, sequence, audio_queue) == GAME_AUDIO_REQUEST_RESULT_OK) {
        projected = true;
    }
    if (game_particle_request_project_from_event(event, tick, sequence, particle_queue) ==
        GAME_PARTICLE_REQUEST_RESULT_OK) {
        projected = true;
    }
    return projected;
}

static GameDefaultSceneResult game_default_scene_run_demo_combat(GameDefaultScene *scene, uint64_t parent_sequence)
{
    GameEntityId target = scene->demo_materialization.actor_count > 0u ? scene->demo_materialization.actor_ids[0]
                                                                       : (GameEntityId){0u, 0u};
    if (!game_entity_id_is_valid(target)) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameCombatAbility abilities[2] = {0};
    GameCombatAbilityCatalog catalog = {0};
    GameCombatCooldownSlot cooldown_slots[2] = {0};
    GameCombatCooldownTable cooldowns = {0};
    if (game_combat_init_catalog(&catalog, abilities, 2u) != GAME_COMBAT_RESULT_OK ||
        game_combat_catalog_set(&catalog, 10u, 320u, 4u, 6) != GAME_COMBAT_RESULT_OK ||
        game_combat_cooldown_init(&cooldowns, cooldown_slots, 2u) != GAME_COMBAT_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameCombatAbilityCommand command = {
        .ability_id = 10u,
        .actor = scene->party_actor,
        .target = target,
        .actor_position = scene->scenario.party_anchor,
        .target_position = scene->scenario.horde_anchor,
    };
    GameCombatResultData combat = {0};
    if (game_combat_resolve_ability(&catalog, &cooldowns, &command, scene->tick, &scene->event_queue, &scene->event_log,
                                    &combat) != GAME_COMBAT_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameProjectileState projectile_slots[2] = {0};
    GameProjectileTable projectiles = {0};
    if (game_projectile_init(&projectiles, projectile_slots, 2u) != GAME_PROJECTILE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    GameProjectileSpawnRequest projectile_request = {
        .source = scene->party_actor,
        .target = target,
        .source_q = scene->scenario.party_anchor.q,
        .source_r = scene->scenario.party_anchor.r,
        .target_q = scene->scenario.horde_anchor.q,
        .target_r = scene->scenario.horde_anchor.r,
        .damage = combat.damage,
        .speed_per_tick = 320u,
        .remaining_lifetime_ticks = 4,
        .payload_id = 1u,
        .ability_id = command.ability_id,
        .parent_event_sequence = combat.ability_event_sequence,
    };
    GameProjectileHandle projectile_handle = {0u, 0u};
    if (game_projectile_spawn(&projectiles, &projectile_request, scene->tick, &projectile_handle) !=
            GAME_PROJECTILE_RESULT_OK ||
        game_projectile_advance_all(&projectiles, scene->tick + 1u, &scene->event_queue, &scene->event_log) !=
            GAME_PROJECTILE_RESULT_OK ||
        game_projectile_advance_all(&projectiles, scene->tick + 2u, &scene->event_queue, &scene->event_log) !=
            GAME_PROJECTILE_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameStatusEffectInstance status_slots[2] = {0};
    GameStatusEffectTable statuses = {0};
    GameStatusEffectHandle status_handle = {0u, 0u};
    if (game_status_effect_init(&statuses, status_slots, 2u) != GAME_STATUS_EFFECT_RESULT_OK ||
        game_status_effect_apply(&statuses, &scene->entities, target, scene->party_actor, GAME_STATUS_EFFECT_TYPE_BURN,
                                 3, 5u, scene->scenario.horde_anchor.q, scene->scenario.horde_anchor.r,
                                 scene->tick + 2u, &scene->event_queue, &scene->event_log, parent_sequence,
                                 &status_handle) != GAME_STATUS_EFFECT_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    GameAudioRequest audio_entries[8] = {0};
    GameAudioRequestQueue audio_queue = {0};
    GameParticleRequest particle_entries[8] = {0};
    GameParticleRequestQueue particle_queue = {0};
    if (game_audio_request_init(&audio_queue, audio_entries, 8u) != GAME_AUDIO_REQUEST_RESULT_OK ||
        game_particle_request_init(&particle_queue, particle_entries, 8u) != GAME_PARTICLE_REQUEST_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    GameEvent event = {0};
    uint64_t sequence = game_default_scene_latest_event_sequence(scene);
    while (game_event_queue_pop(&scene->event_queue, &event) == GAME_EVENT_QUEUE_RESULT_OK) {
        (void)game_default_scene_project_event_requests(&event, sequence, scene->tick + 3u, &audio_queue,
                                                        &particle_queue);
        if (event.type == GAME_EVENT_TYPE_PROJECTILE_IMPACT) {
            scene->demo_status.projectile_impacted = true;
        }
        if (event.type == GAME_EVENT_TYPE_STATUS_EFFECT_APPLIED) {
            scene->demo_status.status_applied = true;
        }
    }

    scene->demo_status.combat_resolved = true;
    scene->demo_status.audio_request_count = (uint32_t)game_audio_request_count(&audio_queue);
    scene->demo_status.particle_request_count = (uint32_t)game_particle_request_count(&particle_queue);
    (void)snprintf(scene->demo_status.combat_summary, sizeof(scene->demo_status.combat_summary),
                   "Combat hit=%s status=%s audio=%u fx=%u", scene->demo_status.projectile_impacted ? "yes" : "no",
                   scene->demo_status.status_applied ? "yes" : "no", scene->demo_status.audio_request_count,
                   scene->demo_status.particle_request_count);
    game_particle_request_destroy(&particle_queue);
    game_audio_request_destroy(&audio_queue);
    scene->demo_status.stages_completed++;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

static GameDefaultSceneResult game_default_scene_run_demo_replay(GameDefaultScene *scene)
{
    GameCommand command = {0};
    command.type = GAME_COMMAND_TYPE_EMIT_NOISE;
    command.source = scene->party_actor;
    command.requested_tick = 0u;
    GameCommandNoisePayload payload = {
        .origin_q = scene->scenario.party_anchor.q,
        .origin_r = scene->scenario.party_anchor.r,
        .intensity = 8,
        .max_radius = 2u,
        .attenuation = 1u,
        .decay_ticks = 3u,
    };
    command.payload_size = sizeof(payload);
    memcpy(command.payload.bytes, &payload, sizeof(payload));
    GameTileFieldConfig field_config = {
        .q_min = -GAME_DEFAULT_SCENE_FIELD_PADDING,
        .r_min = -GAME_DEFAULT_SCENE_FIELD_PADDING,
        .q_count = (size_t)scene->scenario.horde_anchor.q + (size_t)GAME_DEFAULT_SCENE_FIELD_PADDING + 1u,
        .r_count = (size_t)scene->scenario.horde_anchor.r + (size_t)GAME_DEFAULT_SCENE_FIELD_PADDING + 1u,
        .min_value = 0,
        .max_value = 100,
    };
    char trace[512] = {0};
    size_t trace_size = 0u;
    size_t executed = 0u;
    GameReplayInput replay = {
        .seed = scene->seed,
        .ticks_to_run = 1u,
        .commands = &command,
        .command_count = 1u,
        .field_config = &field_config,
    };
    if (game_replay_capture_trace(&replay, trace, sizeof(trace), &trace_size, &executed) != GAME_REPLAY_RESULT_OK) {
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    scene->demo_status.replay_trace_size = trace_size;
    scene->demo_status.replay_commands_executed = executed;
    (void)snprintf(scene->demo_status.replay_summary, sizeof(scene->demo_status.replay_summary),
                   "Replay trace bytes=%zu commands=%zu", trace_size, executed);
    scene->demo_status.stages_completed++;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_run_demo_showcase(GameDefaultScene *scene)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    if (scene->demo_status.showcase_ran) {
        return GAME_DEFAULT_SCENE_RESULT_OK;
    }

    scene->demo_status = (GameDefaultSceneDemoStatus){0};
    uint64_t parent_sequence = game_default_scene_latest_event_sequence(scene);
    if (game_default_scene_run_demo_fields(scene, parent_sequence) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_WARN("Default scene demo fields stage failed.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    if (game_default_scene_run_demo_horde(scene, game_default_scene_latest_event_sequence(scene)) !=
        GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_WARN("Default scene demo horde stage failed.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    if (game_default_scene_run_demo_colony(scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_WARN("Default scene demo colony stage failed.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    if (game_default_scene_run_demo_combat(scene, game_default_scene_latest_event_sequence(scene)) !=
        GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_WARN("Default scene demo combat stage failed.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }
    if (game_default_scene_run_demo_replay(scene) != GAME_DEFAULT_SCENE_RESULT_OK) {
        GAME_LOG_WARN("Default scene demo replay stage failed.");
        return GAME_DEFAULT_SCENE_RESULT_STEP_FAILED;
    }

    scene->demo_status.showcase_ran = true;
    scene->tick += 1u;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_run_opening(GameDefaultScene *scene)
{
    if (!scene) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    GameCommandQueueResult command_result = GAME_COMMAND_QUEUE_RESULT_OK;
    GameDefaultSceneResult result =
        game_default_scene_emit_noise_at(scene, scene->scenario.party_anchor, &command_result);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        return result;
    }
    result = game_default_scene_apply_colony_autonomy(scene);
    if (result != GAME_DEFAULT_SCENE_RESULT_OK) {
        return result;
    }
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

GameDefaultSceneResult game_default_scene_summary(const GameDefaultScene *scene, char *out_buffer, size_t capacity,
                                                  size_t *out_size)
{
    if (!scene || !out_buffer || capacity == 0u || !out_size) {
        return GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT;
    }

    int written =
        snprintf(out_buffer, capacity,
                 "default_scene seed=%llu events=%zu pressure=%u posture=%d worker_audits=%zu passable=%u blocked=%u",
                 (unsigned long long)scene->seed, game_event_log_count(&scene->event_log),
                 scene->horde_attention.pressure, (int)scene->horde_attention.posture, scene->worker_audit_log.count,
                 scene->scenario.passable_tiles, scene->scenario.blocked_tiles);
    if (written < 0 || (size_t)written >= capacity) {
        return GAME_DEFAULT_SCENE_RESULT_BUFFER_TOO_SMALL;
    }
    *out_size = (size_t)written;
    return GAME_DEFAULT_SCENE_RESULT_OK;
}

const GameEventLog *game_default_scene_event_log(const GameDefaultScene *scene)
{
    return scene ? &scene->event_log : NULL;
}

const GameDefaultSceneDemoStatus *game_default_scene_demo_status(const GameDefaultScene *scene)
{
    return scene ? &scene->demo_status : NULL;
}
