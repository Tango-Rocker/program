#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "colony/job_board.h"
#include "colony/worker_ai.h"
#include "ecs/entity.h"
#include "event/event.h"
#include "event/event_log.h"
#include "nav/path_service.h"
#include "sim/horde_group.h"
#include "sim/horde_materialization.h"
#include "sim/horde_attention.h"
#include "sim/command.h"
#include "world/field_registry.h"
#include "world/scenario_gen.h"
#include "world/tile_field.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_DEFAULT_SCENE_MAX_PATH_TILES 2048u

typedef enum {
    GAME_DEFAULT_SCENE_RESULT_OK = 0,
    GAME_DEFAULT_SCENE_RESULT_INVALID_ARGUMENT = 1,
    GAME_DEFAULT_SCENE_RESULT_INIT_FAILED = 2,
    GAME_DEFAULT_SCENE_RESULT_STEP_FAILED = 3,
    GAME_DEFAULT_SCENE_RESULT_BUFFER_TOO_SMALL = 4,
} GameDefaultSceneResult;

typedef enum {
    GAME_DEFAULT_SCENE_INTERACTION_NONE = 0,
    GAME_DEFAULT_SCENE_INTERACTION_TERRAIN = 1,
    GAME_DEFAULT_SCENE_INTERACTION_PARTY = 2,
    GAME_DEFAULT_SCENE_INTERACTION_COLONY = 3,
    GAME_DEFAULT_SCENE_INTERACTION_STOCKPILE = 4,
    GAME_DEFAULT_SCENE_INTERACTION_WORKER = 5,
    GAME_DEFAULT_SCENE_INTERACTION_HORDE = 6,
    GAME_DEFAULT_SCENE_INTERACTION_NOISE = 7,
} GameDefaultSceneInteractionKind;

typedef enum {
    GAME_DEFAULT_SCENE_ATTACK_NONE = 0,
    GAME_DEFAULT_SCENE_ATTACK_HIT = 1,
    GAME_DEFAULT_SCENE_ATTACK_INVALID_TARGET = 2,
} GameDefaultSceneAttackResult;

typedef struct {
    bool showcase_ran;
    uint32_t stages_completed;
    GameHexAxial horde_spawn_tile;
    bool horde_materialized;
    uint32_t horde_spawned_count;
    uint32_t horde_group_pressure;
    uint32_t horde_group_mass;
    uint32_t light_sample;
    uint32_t scent_sample;
    uint32_t blood_sample;
    uint32_t emergency_audit_count;
    bool worker_interrupted;
    bool worker_resumed;
    bool construction_started;
    uint32_t construction_progress;
    bool combat_resolved;
    bool projectile_impacted;
    bool status_applied;
    uint32_t audio_request_count;
    uint32_t particle_request_count;
    size_t replay_trace_size;
    size_t replay_commands_executed;
    char field_summary[96];
    char horde_summary[96];
    char colony_summary[96];
    char combat_summary[96];
    char replay_summary[96];
} GameDefaultSceneDemoStatus;

typedef struct {
    uint64_t seed;
    uint64_t tick;
    GameScenario scenario;
    GameHexAxial party_position;
    GamePathCostMap path_cost_map;
    GamePathQueryScratch path_scratch;
    uint16_t *path_cost_values;
    uint32_t *path_g_score;
    uint32_t *path_f_score;
    int32_t *path_parent;
    bool *path_open;
    bool *path_closed;
    GamePathService path_service;
    GamePathServiceRequestSlot path_slots[4];
    GameHexAxial *path_result_buffer;
    size_t path_result_capacity;
    GameHexAxial path_preview[GAME_DEFAULT_SCENE_MAX_PATH_TILES];
    size_t path_preview_length;
    bool has_path_preview;
    GameHexAxial path_preview_goal;
    GameHexAxial move_path[GAME_DEFAULT_SCENE_MAX_PATH_TILES];
    size_t move_path_length;
    size_t move_path_cursor;
    bool party_following_path;
    GameHexAxial move_goal;
    GameEntityRegistry entities;
    GameEntityId party_actor;
    GameCommandQueue command_queue;
    GameEventQueue event_queue;
    GameEventLog event_log;
    GameTileField noise_field;
    GameFieldRegistry sensory_fields;
    GameFieldRegistrySlot sensory_field_slots[3];
    GameHordeAttentionState horde_attention;
    GameHordeGroupState demo_horde_group;
    GameHordeMaterializationState demo_materialization;
    GameEntityId demo_horde_actors[4];
    GameJobBoard job_board;
    GameJobBoardOrderSlot job_slots[4];
    GameWorkerState worker;
    GameWorkerSelectionAuditEntry worker_audits[4];
    GameWorkerSelectionAuditLog worker_audit_log;
    char causal_report[1024];
    size_t causal_report_size;
    char interaction_summary[128];
    GameDefaultSceneInteractionKind last_interaction_kind;
    char path_summary[128];
    char command_history[8][96];
    size_t command_history_count;
    char alerts[6][96];
    size_t alert_count;
    GameDefaultSceneAttackResult last_attack_result;
    char attack_summary[128];
    GameDefaultSceneDemoStatus demo_status;
} GameDefaultScene;

GameDefaultSceneResult game_default_scene_init(GameDefaultScene *scene);
void game_default_scene_shutdown(GameDefaultScene *scene);
GameDefaultSceneResult game_default_scene_run_opening(GameDefaultScene *scene);
GameDefaultSceneResult game_default_scene_run_demo_showcase(GameDefaultScene *scene);
GameDefaultSceneResult game_default_scene_update(GameDefaultScene *scene);
GameDefaultSceneResult game_default_scene_preview_path_to(GameDefaultScene *scene, GameHexAxial target);
GameDefaultSceneResult game_default_scene_emit_noise_at(
    GameDefaultScene *scene,
    GameHexAxial origin,
    GameCommandQueueResult *out_command_result
);
GameDefaultSceneResult game_default_scene_move_party_to(
    GameDefaultScene *scene,
    GameHexAxial target,
    GameCommandQueueResult *out_command_result
);
GameDefaultSceneResult game_default_scene_interact_at(
    GameDefaultScene *scene,
    GameHexAxial target,
    GameCommandQueueResult *out_command_result
);
GameDefaultSceneResult game_default_scene_attack_at(
    GameDefaultScene *scene,
    GameHexAxial target,
    GameCommandQueueResult *out_command_result
);
GameDefaultSceneResult game_default_scene_summary(
    const GameDefaultScene *scene,
    char *out_buffer,
    size_t capacity,
    size_t *out_size
);
const GameEventLog *game_default_scene_event_log(const GameDefaultScene *scene);
const GameDefaultSceneDemoStatus *game_default_scene_demo_status(const GameDefaultScene *scene);

#ifdef __cplusplus
}
#endif
