# TICKET ID: COLONY-005-construction-worksite-lifecycle

## Objective

Implement construction worksites as deterministic job targets.

## Context

Construction should create world changes through jobs, resource delivery, scheduled work, and events rather than direct UI mutation.

## Sprint

Sprint 8 - colony logistics and construction.

## Dependencies

- COLONY-004-haul-job-chain
- SCHED-001-deterministic-scheduled-events

## Acceptance criteria

- Worksite state includes target tile, required resources, delivered resources, progress, assigned worker, and planned structure type.
- Worksites generate or consume haul/build jobs through documented APIs.
- Progress updates are tick-based and deterministic.
- Completion emits construction facts and requests world structure placement.
- Headless tests cover resource delivery, progress, completion, abort, and trace output.

## Allowed files

- `src/colony/construction.h`
- `src/colony/construction.c`
- `tests/test_construction.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/09_COLONY_JOBS.md`
- `docs/14_SIMULATION_DOCTRINE.md`

## Out of scope

- Player build placement UI.
- Structure rendering.
- Multi-stage blueprints.
- Building upgrades.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the construction test target or full `sim_tests` if tests remain single-binary.

## Notes

Completion should request world mutation through a system boundary, not edit world storage from UI.
