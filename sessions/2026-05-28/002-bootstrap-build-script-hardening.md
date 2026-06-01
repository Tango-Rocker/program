# Session: Bootstrap build scripts hardening

## Goal

Finish and harden the milestone-0 bootstrap workflow by completing `CORE-001` and `TOOL-002` deliverables without changing broader simulation architecture.

## Scope

- Improve bootstrap scripts for deterministic local builds/tests.
- Keep SDL optional and headless-friendly in default workflows.
- Record ticket progression and completion state.

## Files touched

- scripts/configure_debug.sh
- scripts/build_debug.sh
- scripts/test.sh
- backlog/active.md
- backlog/todo.md
- backlog/done.md
- sessions/2026-05-28/002-bootstrap-build-script-hardening.md

## Decisions

- Kept scripts small and shell-portable for Unix-like workflows.
- Added environment defaults for configure-time options (`GAME_ENABLE_SDL`, `GAME_ENABLE_TESTS`).
- Preserved explicit command-line overrides in `configure_debug.sh`.
- Fixed `build_debug.sh` CPU detection to use nproc/getconf and respect provided job override.
- Fixed multi-config test runner flag handling in `test.sh` with clearer config path.
- Deferred broader build system refactor; only local bootstrap/scripting ticket scope was modified.

## Tests

- Not run in this pass (per instruction to avoid extra verification unless explicitly requested).

## Next tasks

- CORE-002-fixed-timestep-loop
- TOOL-001-clangd-compile-commands
- Remaining core/ecs/event/tool bootstrap tickets
