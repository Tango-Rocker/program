# TICKET ID: WORLD-002-hex-coordinate-module

## Objective

Implement axial/cube hex coordinate helpers and deterministic chunk/local indexing helpers.

## Context

`WORLD-001` documented hex conversion and indexing. Navigation, fields, and horde pressure need a tested world coordinate module before higher-level systems are added.

## Sprint

Sprint 1 - deterministic simulation kernel.

## Acceptance criteria

- Axial and cube coordinate structs use explicit integer fields.
- Conversion between axial and cube coordinates follows the documented formula.
- Neighbor lookup returns the six documented axial neighbors in stable order.
- Hex distance matches cube-coordinate distance.
- Chunk key and local offset helpers are deterministic for positive and negative coordinates.
- Headless tests cover conversion, neighbors, distance, and chunk/local indexing.

## Allowed files

- `src/world/hex.h`
- `src/world/hex.c`
- `tests/test_hex.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/07_NAVIGATION.md`
- `docs/12_TESTING.md`

## Out of scope

- Tile storage.
- Pathfinding.
- Rendering/camera coordinate conversion.
- Save/load.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the hex/world test target or full `sim_tests` if tests remain single-binary.

## Notes

Negative coordinate floor division must be explicit. Do not rely on implementation-surprising truncation behavior.
