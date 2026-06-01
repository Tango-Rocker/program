cmake_minimum_required(VERSION 3.20)

project(simulation_horde_rts_rpg C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GAME_ENABLE_SDL "Build SDL app target" ON)
option(GAME_ENABLE_TESTS "Build tests" ON)
option(GAME_ENABLE_SANITIZERS "Enable sanitizer flags where supported" OFF)

enable_testing()

set(SRC_CORE
    src/core/log.c
    src/core/arena.c
    src/core/random.c
    src/core/time.c
)

set(SRC_ECS
    src/ecs/entity.c
)

set(SRC_EVENT
    src/event/event.c
    src/event/event_log.c
)

set(SRC_SIM
    src/sim/sim_context.c
    src/sim/command.c
    src/sim/party.c
    src/sim/combat.c
    src/sim/projectile.c
    src/sim/status_effect.c
    src/sim/noise_system.c
    src/sim/horde_attention.c
    src/sim/replay.c
    src/sim/scheduler.c
    src/sim/snapshot.c
    src/sim/sensory_fields.c
    src/sim/horde_group.c
    src/sim/horde_materialization.c
)

set(SRC_RENDER
    src/render/camera.c
    src/render/hex_debug_render.c
    src/render/particle_request.c
)

set(SRC_AUDIO
    src/audio/audio_request.c
)

set(SRC_WORLD
    src/world/hex.c
    src/world/world_map.c
    src/world/tile_field.c
    src/world/field_registry.c
    src/world/structure.c
    src/world/topology.c
    src/world/scenario_gen.c
)

set(SRC_NAV
    src/nav/pathfind.c
    src/nav/path_service.c
)

set(SRC_COLONY
    src/colony/job_board.c
    src/colony/worker_ai.c
    src/colony/inventory.c
    src/colony/haul_job.c
    src/colony/construction.c
    src/colony/emergency.c
)

set(SRC_LUA
    src/lua/lua_host.c
    src/lua/content_schema.c
)

set(SRC_GAME
    src/game/default_scene.c
)

set(SRC_UI
    src/ui/ui_state.c
    src/ui/command_inspector.c
    src/ui/field_overlay.c
    src/ui/causal_report.c
)

set(SRC_TEST
    tests/test_main.c
    tests/test_world_map.c
    tests/test_arena.c
    tests/test_random.c
    tests/test_ecs_entity.c
    tests/test_event_queue.c
    tests/test_event_log.c
    tests/test_hex.c
    tests/test_command_queue.c
    tests/test_tile_field.c
    tests/test_noise_field.c
    tests/test_field_registry.c
    tests/test_horde_attention.c
    tests/test_sensory_fields.c
    tests/test_horde_group.c
    tests/test_horde_materialization.c
    tests/test_scheduler.c
    tests/test_snapshot.c
    tests/test_pathfind.c
    tests/test_path_service.c
    tests/test_job_board.c
    tests/test_worker_jobs.c
    tests/test_party.c
    tests/test_combat_ability.c
    tests/test_projectile.c
    tests/test_status_effect.c
    tests/test_particle_request.c
    tests/test_inventory.c
    tests/test_haul_job.c
    tests/test_construction.c
    tests/test_structure.c
    tests/test_colony_emergency.c
    tests/test_audio_request.c
    tests/test_replay.c
    tests/test_replay_golden.c
    tests/test_lua_host.c
    tests/test_content_schema.c
    tests/test_sim_tick.c
    tests/test_topology.c
    tests/test_camera.c
    tests/test_command_inspector.c
    tests/test_field_overlay.c
    tests/test_causal_report.c
    tests/test_scenario_gen.c
    tests/test_default_scene.c
    tests/test_ui_state.c
)

add_library(sim_core
    ${SRC_CORE}
    ${SRC_ECS}
    ${SRC_EVENT}
    ${SRC_COLONY}
    ${SRC_WORLD}
    ${SRC_NAV}
    ${SRC_SIM}
    ${SRC_RENDER}
    ${SRC_AUDIO}
    ${SRC_LUA}
    ${SRC_GAME}
    ${SRC_UI}
)

target_include_directories(sim_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(sim_core PRIVATE /W4)
else()
    target_compile_options(sim_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(sim_headless
    src/game/main.c
    src/platform/app.c
)
target_link_libraries(sim_headless PRIVATE sim_core)
target_include_directories(sim_headless PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(MSVC)
    target_compile_options(sim_headless PRIVATE /W4)
else()
    target_compile_options(sim_headless PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(GAME_ENABLE_TESTS)
    add_executable(sim_tests
        ${SRC_TEST}
    )
    target_include_directories(sim_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(sim_tests PRIVATE sim_core)
    if(MSVC)
        target_compile_options(sim_tests PRIVATE /W4)
    else()
        target_compile_options(sim_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME sim_tests COMMAND sim_tests)
    set_tests_properties(sim_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    if(GAME_ENABLE_SANITIZERS AND NOT WIN32)
        target_compile_options(sim_tests PRIVATE -fsanitize=address,undefined)
        target_link_options(sim_tests PRIVATE -fsanitize=address,undefined)
    endif()
endif()

set(HAVE_SDL OFF)
if(GAME_ENABLE_SDL)
    find_package(SDL3 QUIET CONFIG)
    find_package(SDL3_ttf QUIET CONFIG)
    if(SDL3_FOUND AND SDL3_ttf_FOUND)
        set(HAVE_SDL ON)
    elseif(SDL3_FOUND)
        message(WARNING "SDL3_ttf not found. SDL app target skipped. Install mingw-w64-ucrt-x86_64-sdl3-ttf or configure with -DGAME_ENABLE_SDL=OFF.")
    else()
        message(WARNING "SDL3 not found. SDL app target skipped. Configure with -DGAME_ENABLE_SDL=OFF for headless-only builds.")
    endif()
endif()

if(HAVE_SDL)
    add_executable(sim_app
        src/game/main.c
        src/platform/app.c
        src/platform/sdl_app.c
        src/ui/sdl_hud.c
    )
    target_link_libraries(sim_app PRIVATE sim_core SDL3::SDL3 SDL3_ttf::SDL3_ttf)
    target_include_directories(sim_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_definitions(sim_app PRIVATE GAME_WITH_SDL3)
    if(MSVC)
        target_compile_options(sim_app PRIVATE /W4)
    else()
        target_compile_options(sim_app PRIVATE -Wall -Wextra -Wpedantic)
    endif()
else()
    message(STATUS "HEADLESS BUILD: SDL app target not built.")
endif()
