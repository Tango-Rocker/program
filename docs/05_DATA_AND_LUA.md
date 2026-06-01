# 05_DATA_AND_LUA

Lua defines data and high-level behavior.
C owns hot loops, memory, ECS, navigation, rendering, audio, deterministic simulation.
Lua does not directly mutate arbitrary ECS memory.

## Boundary rules

- Lua can define:
  - item and ability metadata
  - behavior parameters
  - progression tables
  - scripted scenario scripts
- C owns:
  - deterministic scheduling
  - memory/arena ownership
  - entity storage and mutation
  - pathing and topology updates
  - authoritative combat and state transitions

## Planned file layout

- `data/lua/` for authored content.
- `src/lua/` for script host and bindings.
- Lua calls create commands/events, not direct state edits.

## Current implementation notes

- `data/lua/prototypes.lua` stores a deterministic worker prototype fixture.
- `src/lua/lua_host.[ch]` provides headless-friendly parsing and immutable prototype records.
- Parse results are explicit and reject missing files and malformed fixtures.
- `src/lua/content_schema.[ch]` validates ability, job, and status-effect prototype rows before updating active C-facing prototype counts.
- `schema/content_schema.md` documents the current fail-closed content row format.
