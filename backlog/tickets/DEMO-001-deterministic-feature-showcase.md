# TICKET ID: DEMO-001-deterministic-feature-showcase

## Objective

Create a deterministic demo slice that showcases the completed systems through the default SDL scene.

## Context

The demo should show the world reacting to player intent while preserving the architecture: UI stages commands, systems mutate state, events record consequences, and forensic panels explain the chain.

## Sprint

Post-sprint demo readiness.

## Dependencies

- UI-005-real-sdl-player-hud
- FIELD-002-light-scent-blood-field-updates
- HORDE-004-materialization-and-dematerialization-rules
- COLONY-005-construction-worksite-lifecycle
- COLONY-006-emergency-threat-interruptions
- COMBAT-002-projectile-lifecycle-and-impact-events
- BUFF-001-status-effect-lifecycle
- AUDIO-001-event-to-audio-request-bus
- PARTICLE-001-particle-request-events
- REPLAY-002-golden-scenario-replay-fixtures

## Acceptance criteria

- `sim_app` opens into a default scene with a completed demo chain summary in the HUD.
- The demo chain uses existing systems for sensory fields, horde group escalation/materialization, colony emergency/construction, combat ability, projectile impact, status effect, audio request projection, particle request projection, and replay trace capture.
- `Emit Noise` remains interactive and continues to use the command inspector/queue/noise/horde/event/causal chain.
- Headless tests assert the demo status proves each feature beat ran.
- Documentation and session archive describe the demo path.

## Allowed files

- `src/game/default_scene.[ch]`
- `src/game/main.c`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.c`
- `tests/test_default_scene.c`
- `README.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `backlog/active.md`
- `backlog/done.md`
- `sessions/2026-05-30/022-demo-feature-showcase.md`

## Out of scope

- New production art, minimap, inventory UI, full combat controls, or non-deterministic scripted presentation.

## Required checks

- `.\scripts\verify-debug.ps1`
- `cmake --build cmake-build-debug --target sim_app`
- `.\cmake-build-debug\sim_tests.exe`
- `ctest --test-dir cmake-build-debug --output-on-failure`

## Status

Complete.
