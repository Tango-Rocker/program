# TICKET ID: LUA-002-content-driven-ability-and-job-prototypes

## Objective

Wire validated content prototypes into ability and job systems.

## Context

The first Lua boundary only proves loading. This ticket makes authored data influence existing systems through immutable prototype values.

## Sprint

Sprint 10 - data, scenarios, and golden replay coverage.

## Dependencies

- DATA-001-content-schema-and-validator
- COMBAT-001-ability-command-resolution
- COLONY-002-job-board-storage-and-reservations

## Acceptance criteria

- Ability resolution can read range, cooldown, and payload values from validated prototypes.
- Job creation can read role/resource/duration values from validated prototypes.
- Missing prototype ids fail validation or command/job creation explicitly.
- Prototype lookup ordering is deterministic.
- Headless tests cover content-driven ability, content-driven job, missing id, and unchanged behavior after failed load.

## Allowed files

- `src/lua/content_db.h`
- `src/lua/content_db.c`
- `src/sim/combat.c`
- `src/colony/job_board.c`
- `tests/test_content_driven_systems.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/05_DATA_AND_LUA.md`

## Out of scope

- Runtime script behavior.
- UI content browser.
- Save compatibility for content changes.
- Mod package loading.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the content-driven systems test target or full `sim_tests` if tests remain single-binary.

## Notes

Lua/data may choose values. C systems still own validation, scheduling, and authoritative mutation.
