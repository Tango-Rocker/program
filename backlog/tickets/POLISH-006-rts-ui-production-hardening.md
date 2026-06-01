# TICKET ID: POLISH-006-rts-ui-production-hardening

## Objective

Harden the current RTS UI/input demo against the highest-risk game-development review findings.

## Context

The player-facing HUD and controls are functional, but the industry review identified correctness and production-readiness gaps around combat target identity, event ownership, compact layout, large-map rendering cost, path-preview hitches, text rendering churn, and simulation-time display.

## Sprint

Post-sprint demo hardening pass.

## Dependencies

- PLAYER-002-rts-input-polish
- PLAYER-003-path-preview-and-follow-movement
- PLAYER-005-tactical-attack-command
- UI-005-real-sdl-player-hud
- UI-008-pause-speed-minimap-filter-inspector

## Acceptance criteria

- Attack commands cannot resolve against the party or an abstract marker when no materialized hostile actor exists.
- UI command entry points do not clear pending event queue data.
- Compact layouts keep action buttons and forensic toggles inside the visible non-inspector area.
- World rendering and world hit-testing use visible tile bounds instead of scanning the whole map.
- SDL path preview requests are debounced/cached by party origin and target tile.
- SDL HUD text rendering caches text textures across frames.
- The top bar displays authoritative simulation tick instead of app/frame tick.
- UI camera control is routed through the existing camera state where the SDL app owns camera movement.
- Headless tests cover the simulation/UI invariants that can be tested without SDL.
- SDL app target builds after renderer/input changes.

## Allowed files

- `src/game/**`
- `src/ui/**`
- `tests/**`
- `docs/**`
- `backlog/**`
- `sessions/**`

## Out of scope

- Full production renderer/chunk streaming.
- New art, animation, audio, or ability balance.
- Full UI automation/screenshot test harness.

## Required checks

- `cmake --build cmake-build-debug --target sim_tests`
- `.\cmake-build-debug\sim_tests.exe`
- `cmake --build cmake-build-debug --target sim_app`
- `ctest --test-dir cmake-build-debug --output-on-failure`
