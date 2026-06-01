# TICKET ID: WORLD-004-chunked-world-map-storage

## Objective

Implement chunked world tile storage keyed by deterministic hex chunk coordinates.

## Context

The world currently has coordinate and bounded field primitives. Larger simulation systems need a stable map store that can grow beyond single bounded fixtures while preserving deterministic indexing.

## Sprint

Sprint 5 - world scale, scheduling, and persistence.

## Dependencies

- WORLD-002-hex-coordinate-module
- WORLD-003-tile-field-storage

## Acceptance criteria

- Chunk keys and local tile offsets use the documented axial/cube conversion rules.
- Tile storage supports create/get/update queries through explicit result codes.
- Iteration order across chunks and local tiles is stable.
- Missing chunks fail closed and do not allocate implicitly unless the API name documents creation.
- Headless tests cover positive and negative coordinates, chunk boundary transitions, stable iteration, and missing chunk behavior.

## Allowed files

- `src/world/world_map.h`
- `src/world/world_map.c`
- `tests/test_world_map.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/03_MODULE_MAP.md`
- `docs/07_NAVIGATION.md`
- `docs/12_TESTING.md`

## Out of scope

- Procedural generation.
- Rendering.
- Save/load serialization.
- Infinite streaming.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the world map test target or full `sim_tests` if tests remain single-binary.

## Notes

Avoid hidden global world state. Prefer caller-owned storage or explicit context ownership.
