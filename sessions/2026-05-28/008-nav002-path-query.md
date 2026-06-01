# Session: Implement NAV-002 hex A* path query

## Goal

Complete `NAV-002-hex-a-star-path-query` as the next Sprint 3 ticket.

## Scope

- Added synchronous bounded A* path query over axial coordinates.
- Added explicit cost grid and blocked cost semantics.
- Added deterministic neighbor expansion order and deterministic tie-break when scores tie.
- Added explicit failure modes for invalid input, out-of-bounds, no route, budget exhaustion, and path buffer too small.
- Added deterministic path output reconstruction.
- Added headless coverage for:
  - straight route,
  - obstacle detour,
  - unreachable goal,
  - budget exhaustion,
  - stable tie-break,
  - output-too-small.
- Wired pathfinding tests into `test_main.c`.
- Added `src/nav/pathfind.c/h` to `sim_core`.

## Files touched

- src/nav/pathfind.h
- src/nav/pathfind.c
- tests/test_pathfind.c
- tests/test_main.c
- CMakeLists.txt
- docs/07_NAVIGATION.md
- docs/12_TESTING.md
- backlog/active.md
- backlog/todo.md
- backlog/done.md
- backlog/sprints.md

## Decisions

- Kept scratch memory caller-owned to avoid heap allocation in the hot loop.
- Used axial hex neighbor order from `world/hex` to avoid introducing a new ordering convention.
- Represented passability/cost in a compact bounded cost map for deterministic testability.

## Tests

- Added `test_pathfind` and integrated it into the `sim_tests` executable.
- Full build/test execution not run (no explicit request).

## Next tasks

- Start `NAV-003-path-request-service-budgeting` with the new path query as its synchronous primitive.
