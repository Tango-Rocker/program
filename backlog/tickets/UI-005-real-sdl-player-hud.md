# TICKET ID: UI-005-real-sdl-player-hud

## Objective

Build the first player-facing SDL HUD around the default scene while keeping UI state non-authoritative.

## Context

The default scene should open to a readable world view with selection, panels, forensic traces, and one real command path. UI may stage and submit commands, but simulation mutation must continue through commands, systems, events, and reports.

## Sprint

Post-sprint demo readiness.

## Dependencies

- UI-002-command-staging-and-inspector
- UI-003-field-overlay-debug-views
- UI-004-after-action-causal-report
- PROC-001-seeded-scenario-map-generator

## Acceptance criteria

- `sim_app` renders a player HUD with hex-ish map geometry, top status bar, bottom party/action panel, right inspector, event strip, and collapsible forensic controls.
- Left-click selection uses marker priority over tile hit-tests.
- `Emit Noise` submits through `GameCommandInspector` and the default scene command queue path.
- Command submission updates the noise field, horde pressure, event log, and causal report through existing simulation systems.
- SDL3_ttf text rendering uses Windows UI fonts when available and falls back to shape-only rendering with a warning.
- Headless tests cover UI layout/hit-testing and the default-scene command surface.

## Allowed files

- `src/game/default_scene.[ch]`
- `src/game/main.c`
- `src/platform/app.h`
- `src/platform/sdl_app.c`
- `src/ui/ui_state.[ch]`
- `src/ui/sdl_hud.[ch]`
- `tests/test_ui_state.c`
- `tests/test_default_scene.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `docs/13_KNOWN_ISSUES.md`
- `backlog/active.md`
- `sessions/2026-05-30/021-real-sdl-ui-hud.md`

## Out of scope

- Production art, animation, inventory UI, minimap, SDL_image, and broad game controls.

## Required checks

- `.\scripts\verify-debug.ps1`
- `cmake --build cmake-build-debug --target sim_app`
- `.\cmake-build-debug\sim_tests.exe`
- `ctest --test-dir cmake-build-debug --output-on-failure`

## Status

Complete.
