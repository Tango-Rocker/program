# TICKET ID: RENDER-001-camera-transform-and-party-leash

## Objective

Implement camera transform math and party leash policy outside authoritative simulation state.

## Context

The player camera is party-bound with a leash, but render/camera state must not mutate simulation state.

## Sprint

Sprint 9 - player-facing debug and control surfaces.

## Dependencies

- PARTY-001-party-actor-state-and-selection

## Acceptance criteria

- Camera state stores position, zoom, viewport size, and leash target.
- World-to-screen and screen-to-world transforms are deterministic for test inputs.
- Leash policy follows selected party position without changing party position.
- Headless tests cover transform round-trip, zoom bounds, leash clamping, and no simulation mutation.

## Allowed files

- `src/render/camera.h`
- `src/render/camera.c`
- `tests/test_camera.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/08_UI_UX.md`

## Out of scope

- SDL input handling.
- Smooth animation.
- Minimap.
- Render batching.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the camera test target or full `sim_tests` if tests remain single-binary.

## Notes

Camera code can be deterministic without being part of replay-authoritative simulation.
