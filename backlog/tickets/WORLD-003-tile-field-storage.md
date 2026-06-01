# TICKET ID: WORLD-003-tile-field-storage

## Objective

Implement bounded deterministic tile field storage for scalar simulation fields.

## Context

Noise, threat, smell, heat, and other world-hears-you signals need a shared field primitive before domain-specific systems are layered on top.

## Sprint

Sprint 2 - first causal world-hears-you chain.

## Dependencies

- WORLD-002-hex-coordinate-module

## Acceptance criteria

- Field storage maps hex coordinates to scalar values within an explicitly configured bounded area.
- Get/set/add operations return explicit result codes for out-of-bounds coordinates.
- Values clamp to documented min/max limits.
- Decay/update pass is deterministic and does not allocate in the hot loop unless explicitly documented.
- Headless tests cover get/set/add, clamping, out-of-bounds behavior, and decay ordering.

## Allowed files

- `src/world/tile_field.h`
- `src/world/tile_field.c`
- `tests/test_tile_field.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/14_SIMULATION_DOCTRINE.md`
- `docs/12_TESTING.md`

## Out of scope

- Infinite world streaming.
- Multiple field types beyond scalar storage.
- Rendering overlays.
- Save/load.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the tile field test target or full `sim_tests` if tests remain single-binary.

## Notes

Prefer a deliberately bounded first implementation over a broad sparse-world abstraction. The API should not prevent later chunk-backed storage.
