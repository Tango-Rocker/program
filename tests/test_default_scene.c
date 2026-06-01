#include <stdio.h>
#include <string.h>

#include "game/default_scene.h"

static int assert_true(int condition, const char *label)
{
    if (!condition) {
        printf("[default_scene] FAIL: %s\n", label);
        return 1;
    }
    return 0;
}

int test_default_scene(void)
{
    int failed = 0;
    GameDefaultScene scene = {0};
    failed += assert_true(game_default_scene_init(&scene) == GAME_DEFAULT_SCENE_RESULT_OK, "scene init");
    failed += assert_true(scene.scenario.horde_anchor.q >= 179, "default scene uses 100x wider showcase map area");
    failed += assert_true(scene.scenario.horde_anchor.r >= 119, "default scene uses 100x taller showcase map area");
    failed += assert_true(scene.scenario.passable_tiles + scene.scenario.blocked_tiles >= 21600u,
                          "showcase map has 100x tile count");
    failed += assert_true(scene.party_position.q == scene.scenario.party_anchor.q, "party starts at scenario anchor");
    failed += assert_true(game_default_scene_run_opening(&scene) == GAME_DEFAULT_SCENE_RESULT_OK, "opening run");
    failed += assert_true(game_event_log_count(game_default_scene_event_log(&scene)) == 3u, "three causal events");
    failed += assert_true(scene.horde_attention.posture == GAME_HORDE_POSTURE_THREATENED, "horde reacts");
    failed += assert_true(scene.worker_audit_log.count == 1u, "worker autonomy audited");
    failed += assert_true(scene.worker.has_current_job, "worker selected default job");
    failed += assert_true(strstr(scene.causal_report, "type=3") != NULL, "report contains noise root");
    failed += assert_true(strstr(scene.causal_report, "type=5") != NULL, "report contains attention reaction");

    char summary[256] = {0};
    size_t summary_size = 0u;
    failed += assert_true(game_default_scene_summary(&scene, summary, sizeof(summary), &summary_size) ==
                              GAME_DEFAULT_SCENE_RESULT_OK,
                          "summary writes");
    failed += assert_true(strstr(summary, "default_scene seed=1337") != NULL, "summary names default scene");

    size_t before_events = game_event_log_count(game_default_scene_event_log(&scene));
    size_t before_queue = game_event_queue_count(&scene.event_queue);
    uint32_t before_pressure = scene.horde_attention.pressure;
    GameCommandQueueResult command_result = GAME_COMMAND_QUEUE_RESULT_OK;
    failed += assert_true(game_default_scene_emit_noise_at(&scene, (GameHexAxial){1, 0}, &command_result) ==
                              GAME_DEFAULT_SCENE_RESULT_OK,
                          "ui command path emits noise");
    failed += assert_true(command_result == GAME_COMMAND_QUEUE_RESULT_OK, "ui command path uses command queue");
    failed += assert_true(game_event_log_count(game_default_scene_event_log(&scene)) > before_events,
                          "ui command path appends events");
    failed += assert_true(game_event_queue_count(&scene.event_queue) > before_queue,
                          "noise preserves and appends queued events");
    failed += assert_true(scene.horde_attention.pressure > before_pressure, "ui command path updates horde pressure");
    failed += assert_true(strstr(scene.causal_report, "type=5") != NULL, "ui command path refreshes causal report");

    GameHexAxial move_target = {3, 0};
    failed += assert_true(game_default_scene_preview_path_to(&scene, move_target) == GAME_DEFAULT_SCENE_RESULT_OK,
                          "path preview resolves");
    failed +=
        assert_true(scene.has_path_preview && scene.path_preview_length >= 2u, "path preview stores visible route");
    failed += assert_true(strstr(scene.path_summary, "Path preview") != NULL, "path preview summary updates");

    before_events = game_event_log_count(game_default_scene_event_log(&scene));
    before_queue = game_event_queue_count(&scene.event_queue);
    failed += assert_true(game_default_scene_move_party_to(&scene, move_target, &command_result) ==
                              GAME_DEFAULT_SCENE_RESULT_OK,
                          "ui move command path starts path-follow movement");
    failed += assert_true(command_result == GAME_COMMAND_QUEUE_RESULT_OK, "move command uses command queue");
    failed += assert_true(scene.party_following_path, "move command follows resolved path");
    failed += assert_true(game_event_log_count(game_default_scene_event_log(&scene)) == before_events,
                          "move order does not teleport or append movement event before stepping");
    failed += assert_true(game_event_queue_count(&scene.event_queue) == before_queue,
                          "move order does not clear queued events");
    for (int step = 0; step < 16 && scene.party_following_path; ++step) {
        failed += assert_true(game_default_scene_update(&scene) == GAME_DEFAULT_SCENE_RESULT_OK, "path step updates");
    }
    failed += assert_true(!scene.party_following_path, "path-follow movement completes");
    failed += assert_true(scene.party_position.q == move_target.q && scene.party_position.r == move_target.r,
                          "party reaches path goal");
    failed += assert_true(game_event_log_count(game_default_scene_event_log(&scene)) > before_events,
                          "path-follow movement appends movement events");

    before_events = game_event_log_count(game_default_scene_event_log(&scene));
    before_queue = game_event_queue_count(&scene.event_queue);
    failed += assert_true(game_default_scene_interact_at(&scene, scene.scenario.colony_anchor, &command_result) ==
                              GAME_DEFAULT_SCENE_RESULT_OK,
                          "ui interaction command path records world interaction");
    failed += assert_true(command_result == GAME_COMMAND_QUEUE_RESULT_OK, "interact command uses command queue");
    failed += assert_true(game_event_log_count(game_default_scene_event_log(&scene)) == before_events + 1u,
                          "interact appends event");
    failed += assert_true(game_event_queue_count(&scene.event_queue) == before_queue,
                          "interact does not clear queued events");
    failed += assert_true(scene.last_interaction_kind == GAME_DEFAULT_SCENE_INTERACTION_COLONY,
                          "colony interaction is contextual");
    failed +=
        assert_true(strstr(scene.interaction_summary, "Colony hub") != NULL, "contextual interaction summary updates");
    failed += assert_true(game_default_scene_interact_at(&scene, scene.worker.position, &command_result) ==
                              GAME_DEFAULT_SCENE_RESULT_OK,
                          "worker interaction command path records context");
    failed += assert_true(scene.last_interaction_kind == GAME_DEFAULT_SCENE_INTERACTION_WORKER,
                          "worker interaction is contextual");

    before_events = game_event_log_count(game_default_scene_event_log(&scene));
    before_queue = game_event_queue_count(&scene.event_queue);
    failed += assert_true(game_default_scene_attack_at(&scene, scene.scenario.horde_anchor, &command_result) ==
                              GAME_DEFAULT_SCENE_RESULT_STEP_FAILED,
                          "attack rejects horde marker before hostile actor materializes");
    failed +=
        assert_true(command_result == GAME_COMMAND_QUEUE_RESULT_BAD_PAYLOAD, "invalid attack reports bad payload");
    failed += assert_true(scene.last_attack_result == GAME_DEFAULT_SCENE_ATTACK_INVALID_TARGET,
                          "invalid attack result records miss");
    failed += assert_true(game_event_log_count(game_default_scene_event_log(&scene)) == before_events,
                          "invalid attack does not append combat events");
    failed += assert_true(game_event_queue_count(&scene.event_queue) == before_queue,
                          "invalid attack does not clear queued events");

    before_events = game_event_log_count(game_default_scene_event_log(&scene));
    failed +=
        assert_true(game_default_scene_run_demo_showcase(&scene) == GAME_DEFAULT_SCENE_RESULT_OK, "demo showcase runs");
    const GameDefaultSceneDemoStatus *demo = game_default_scene_demo_status(&scene);
    failed += assert_true(demo != NULL && demo->showcase_ran, "demo status available");
    failed += assert_true(demo != NULL && demo->stages_completed == 5u, "demo completes all beats");
    failed += assert_true(demo != NULL && demo->light_sample > 0u, "demo applies sensory fields");
    failed += assert_true(demo != NULL && demo->horde_materialized, "demo materializes horde actors");
    failed += assert_true(demo != NULL && demo->worker_interrupted, "demo interrupts colony routine");
    failed += assert_true(demo != NULL && demo->worker_resumed, "demo resumes colony routine");
    failed += assert_true(demo != NULL && demo->construction_started, "demo starts construction workflow");
    failed += assert_true(demo != NULL && demo->combat_resolved, "demo resolves combat ability");
    failed += assert_true(demo != NULL && demo->projectile_impacted, "demo advances projectile impact");
    failed += assert_true(demo != NULL && demo->status_applied, "demo applies status effect");
    failed += assert_true(demo != NULL && demo->audio_request_count > 0u, "demo projects audio requests");
    failed += assert_true(demo != NULL && demo->particle_request_count > 0u, "demo projects particle requests");
    failed += assert_true(demo != NULL && demo->replay_commands_executed == 1u, "demo captures replay trace");
    failed += assert_true(game_event_log_count(game_default_scene_event_log(&scene)) > before_events,
                          "demo appends trace events");

    before_events = game_event_log_count(game_default_scene_event_log(&scene));
    before_queue = game_event_queue_count(&scene.event_queue);
    failed += assert_true(demo != NULL && game_default_scene_attack_at(&scene, demo->horde_spawn_tile,
                                                                       &command_result) == GAME_DEFAULT_SCENE_RESULT_OK,
                          "attack command resolves against materialized horde");
    failed += assert_true(command_result == GAME_COMMAND_QUEUE_RESULT_OK, "attack command uses command queue");
    failed += assert_true(scene.last_attack_result == GAME_DEFAULT_SCENE_ATTACK_HIT, "attack result records hit");
    failed += assert_true(strstr(scene.attack_summary, "Attack hit") != NULL, "attack summary updates");
    failed += assert_true(scene.command_history_count > 0u, "command history records commands");
    failed += assert_true(scene.alert_count > 0u, "alerts record command consequences");
    failed += assert_true(game_event_log_count(game_default_scene_event_log(&scene)) > before_events,
                          "attack appends combat events");
    failed += assert_true(game_event_queue_count(&scene.event_queue) > before_queue,
                          "attack preserves and appends queued events");
    failed += assert_true(game_default_scene_run_demo_showcase(&scene) == GAME_DEFAULT_SCENE_RESULT_OK,
                          "demo showcase is idempotent");

    game_default_scene_shutdown(&scene);
    if (failed == 0) {
        printf("[default_scene] PASS\n");
    }
    return failed;
}
