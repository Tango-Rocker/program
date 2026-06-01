#include <stdio.h>
#include <string.h>

int test_arena(void);
int test_random(void);
int test_entity_registry(void);
int test_event_queue(void);
int test_hex(void);
int test_sim_tick(void);
int test_command_queue(void);
int test_tile_field(void);
int test_field_registry(void);
int test_sensory_fields(void);
int test_event_log(void);
int test_noise_field(void);
int test_horde_attention(void);
int test_horde_group(void);
int test_horde_materialization(void);
int test_scheduler(void);
int test_snapshot(void);
int test_pathfind(void);
int test_path_service(void);
int test_job_board(void);
int test_worker_jobs(void);
int test_inventory(void);
int test_haul_job(void);
int test_construction(void);
int test_structure(void);
int test_colony_emergency(void);
int test_party(void);
int test_combat_ability(void);
int test_projectile(void);
int test_status_effect(void);
int test_particle_request(void);
int test_audio_request(void);
int test_replay(void);
int test_replay_golden(void);
int test_lua_host(void);
int test_content_schema(void);
int test_world_map(void);
int test_topology(void);
int test_camera(void);
int test_command_inspector(void);
int test_field_overlay(void);
int test_causal_report(void);
int test_scenario_gen(void);
int test_default_scene(void);
int test_ui_state(void);

typedef struct {
    const char *name;
    int (*fn)(void);
} TestEntry;

static const TestEntry TESTS[] = {
    {"arena", test_arena},
    {"random", test_random},
    {"entity_registry", test_entity_registry},
    {"event_queue", test_event_queue},
    {"hex", test_hex},
    {"sim_tick", test_sim_tick},
    {"command_queue", test_command_queue},
    {"tile_field", test_tile_field},
    {"field_registry", test_field_registry},
    {"sensory_fields", test_sensory_fields},
    {"world_map", test_world_map},
    {"inventory", test_inventory},
    {"haul_job", test_haul_job},
    {"construction", test_construction},
    {"event_log", test_event_log},
    {"noise_field", test_noise_field},
    {"horde_attention", test_horde_attention},
    {"structure", test_structure},
    {"colony_emergency", test_colony_emergency},
    {"horde_group", test_horde_group},
    {"horde_materialization", test_horde_materialization},
    {"scheduler", test_scheduler},
    {"snapshot", test_snapshot},
    {"pathfind", test_pathfind},
    {"path_service", test_path_service},
    {"job_board", test_job_board},
    {"worker_jobs", test_worker_jobs},
    {"party", test_party},
    {"combat_ability", test_combat_ability},
    {"projectile", test_projectile},
    {"status_effect", test_status_effect},
    {"audio_request", test_audio_request},
    {"particle_request", test_particle_request},
    {"replay", test_replay},
    {"replay_golden", test_replay_golden},
    {"lua_host", test_lua_host},
    {"content_schema", test_content_schema},
    {"topology", test_topology},
    {"camera", test_camera},
    {"command_inspector", test_command_inspector},
    {"field_overlay", test_field_overlay},
    {"causal_report", test_causal_report},
    {"scenario_gen", test_scenario_gen},
    {"default_scene", test_default_scene},
    {"ui_state", test_ui_state},
};

static int run_test(const char *name, int (*test_fn)(void)) {
    printf("START: %s\n", name);
    int failed = test_fn();
    printf("DONE: %s=%d\n", name, failed);
    return failed;
}

int main(int argc, char **argv) {
    (void)setvbuf(stdout, NULL, _IONBF, 0);
    int failed = 0;
    if (argc > 1 && argv[1] != NULL) {
        for (size_t i = 0u; i < sizeof(TESTS) / sizeof(*TESTS); ++i) {
            if (strcmp(argv[1], TESTS[i].name) == 0) {
                failed += run_test(TESTS[i].name, TESTS[i].fn);
                if (failed == 0) {
                    printf("ALL TESTS PASSED\n");
                    return 0;
                }
                printf("%d TESTS FAILED\n", failed);
                return 1;
            }
        }
        printf("UNKNOWN TEST: %s\n", argv[1]);
        return 1;
    }

    for (size_t i = 0u; i < sizeof(TESTS) / sizeof(*TESTS); ++i) {
        failed += run_test(TESTS[i].name, TESTS[i].fn);
    }

    if (failed == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }

    printf("%d TESTS FAILED\n", failed);
    return 1;
}
