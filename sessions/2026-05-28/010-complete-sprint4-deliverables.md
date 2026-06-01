# Session: Complete Sprint 4 Sprint-Critical Deliverables

## Goal

Finish remaining Sprint 4 tickets.

- REPLAY-001-deterministic-input-log-and-replay-shell
- TOOL-003-generated-module-index-prototype
- LUA-001-lua-data-boundary-stub
- UI-001-sdl-debug-event-log-overlay

## Files touched

- src/sim/replay.h
- src/sim/replay.c
- src/lua/lua_host.h
- src/lua/lua_host.c
- src/ui/debug_overlay.h
- src/ui/debug_overlay.c
- src/platform/sdl_app.c
- tests/test_main.c
- tests/test_replay.c
- tests/test_lua_host.c
- CMakeLists.txt
- tools/indexers/module_index.py
- scripts/generate_indexes.sh
- scripts/generate-indexes.ps1
- data/lua/README.md
- data/lua/prototypes.lua
- data/lua/prototypes_invalid.lua
- generated/module_index.md
- docs/00_INDEX.md
- docs/05_DATA_AND_LUA.md
- docs/06_ECS_AND_EVENTS.md
- docs/08_UI_UX.md
- docs/12_TESTING.md
- docs/14_SIMULATION_DOCTRINE.md
- backlog/active.md
- backlog/todo.md
- backlog/done.md
- backlog/sprints.md

## Decisions

- Added a replay fixture runner that uses the existing command queue and noise system to produce deterministic event-trace output.
- Added first-pass line-based mismatch reporting for replay output comparison.
- Implemented minimal deterministic Lua fixture parser for worker prototype rows in a stable, headless-safe format.
- Added generated module index tool and wrapper scripts for repeatable index output.
- Added SDL-only debug overlay renderer that reads event log snapshots without mutating simulation state.
- Kept all additions aligned to ticket allowed files and existing module boundaries.

## Known risks

- Replay currently models noise command playback only, matching currently implemented command coverage.
- Lua fixture format is intentionally strict and intentionally minimal.
- Overlay currently renders simple occupancy bars and does not draw text labels.

## Verification

- Did not run full build/tests in this session to avoid changing already-running state.
- Added test registrations in `tests/test_main.c` for the new `test_replay` and `test_lua_host` suites.
