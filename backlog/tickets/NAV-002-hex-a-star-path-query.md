# TICKET ID: NAV-002-hex-a-star-path-query

## Objective

Implement deterministic hex-grid A* path queries for bounded maps.

## Context

`NAV-001` documented path handles, but there is no path calculation yet. This ticket adds a synchronous path query primitive that later path-service handles can budget and cache.

## Sprint

Sprint 3 - autonomous work and pathing.

## Dependencies

- WORLD-002-hex-coordinate-module

## Acceptance criteria

- Path query accepts start, goal, passability/cost data, and an explicit node budget.
- Neighbor expansion order is stable and documented.
- Tie-breaking is deterministic.
- Failure modes distinguish no route, budget exhausted, invalid input, and output buffer too small.
- Headless tests cover straight path, obstacle detour, unreachable goal, budget exhaustion, and stable tie-break output.

## Allowed files

- `src/nav/pathfind.h`
- `src/nav/pathfind.c`
- `tests/test_pathfind.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/07_NAVIGATION.md`
- `docs/12_TESTING.md`

## Out of scope

- Async path handles.
- Multithreading.
- Flow fields.
- Horde group movement.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the pathfinding test target or full `sim_tests` if tests remain single-binary.

## Notes

Avoid heap allocation in the query hot path unless an explicit caller-owned arena or scratch buffer is used.
