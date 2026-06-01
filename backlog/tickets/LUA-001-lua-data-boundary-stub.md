# TICKET ID: LUA-001-lua-data-boundary-stub

## Objective

Create the first Lua/data boundary stub without allowing arbitrary ECS mutation.

## Context

Lua is planned for content and high-level behavior values, while C owns deterministic hot loops and authoritative simulation state. This ticket creates the boundary before real scripted behavior is added.

## Sprint

Sprint 4 - replay, indexes, data boundary, and debug surface.

## Acceptance criteria

- Lua host or data boundary API exposes explicit load/parse result codes.
- Build remains headless-testable when a Lua runtime is unavailable.
- Data boundary can load or represent a small authored content table fixture.
- API returns immutable content values to simulation code rather than raw script access to ECS mutation.
- Headless tests cover missing file/input, valid fixture load, and invalid fixture rejection.

## Allowed files

- `src/lua/lua_host.h`
- `src/lua/lua_host.c`
- `data/lua/README.md`
- `data/lua/prototypes.lua`
- `tests/test_lua_host.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/05_DATA_AND_LUA.md`
- `docs/12_TESTING.md`

## Out of scope

- Arbitrary behavior scripts.
- Hot ECS mutation from Lua.
- Runtime script reload UI.
- Complex schema validation.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the Lua boundary test target or full `sim_tests` if tests remain single-binary.
- Verify headless build behavior when Lua runtime support is disabled or unavailable.

## Notes

If external Lua linkage is introduced, it must be optional like SDL unless a future ticket changes build requirements.
