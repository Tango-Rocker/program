# TICKET ID: DEMO-002-large-showcase-map

## Objective

Expand the default demo from a compact proving scene into a larger deterministic map that better shows simulation systems acting across distance.

## Context

The feature showcase already proves the causal chain, but the 7x5 map compresses every system into a small clump. The demo should make the party, colony, stockpile, horde edge, fields, horde materialization, and command consequences visibly distributed across a larger world.

## Sprint

Post-sprint demo readiness.

## Dependencies

- DEMO-001-deterministic-feature-showcase
- PROC-001-seeded-scenario-map-generator
- UI-005-real-sdl-player-hud

## Acceptance criteria

- The default scene generates a larger showcase map with at least 216 tiles.
- Noise and sensory field storage derive from the generated map size instead of the old compact 9x9 extent.
- HUD map geometry scales so the larger map fits at 1280x720 and compact viewport layouts.
- Existing marker priority, selection, `Emit Noise`, demo summaries, and forensic controls remain usable.
- Headless tests assert the map size and fit.

## Allowed files

- `src/game/default_scene.c`
- `src/ui/ui_state.c`
- `tests/test_default_scene.c`
- `tests/test_ui_state.c`
- `README.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `backlog/active.md`
- `backlog/done.md`
- `sessions/2026-05-30/023-large-showcase-map.md`

## Required checks

- `.\scripts\verify-debug.ps1`
- `cmake --build cmake-build-debug --target sim_app`
- `.\cmake-build-debug\sim_tests.exe`
- `ctest --test-dir cmake-build-debug --output-on-failure`

## Status

Complete.
