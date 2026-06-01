# TICKET ID: DATA-001-content-schema-and-validator

## Objective

Create a small content schema and validator for authored prototypes.

## Context

Lua/content data should fail closed before it reaches simulation systems. This ticket adds a minimal schema path for abilities, jobs, and effects.

## Sprint

Sprint 10 - data, scenarios, and golden replay coverage.

## Dependencies

- LUA-001-lua-data-boundary-stub

## Acceptance criteria

- Schema defines required fields for at least one ability, one job prototype, and one status effect prototype.
- Validator reports missing fields, invalid ids, and out-of-range values with useful context.
- Valid fixture data loads into immutable C-facing prototype structs.
- Invalid fixture data does not partially update active prototype state.
- Tests cover valid data, missing required field, duplicate id, and range failure.

## Allowed files

- `schema/content_schema.md`
- `src/lua/content_schema.h`
- `src/lua/content_schema.c`
- `data/lua/prototypes.lua`
- `tests/test_content_schema.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/05_DATA_AND_LUA.md`
- `docs/12_TESTING.md`

## Out of scope

- Full mod support.
- Hot reload.
- Localization.
- Complex inheritance between prototypes.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the content schema test target or full `sim_tests` if tests remain single-binary.

## Notes

Keep content ids stable and explicit. Avoid exposing raw Lua tables to hot simulation systems.
