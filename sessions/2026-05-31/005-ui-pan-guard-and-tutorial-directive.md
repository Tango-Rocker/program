# Session 005 - UI Pan Guard and Tutorial Directive

## Goal

Fix camera panning while hovering over UI chrome and establish a standing directive to keep tutorial guidance updated as gameplay and UI change.

## Files touched

- `src/game/main.c`
- `src/ui/ui_state.h`
- `src/ui/ui_state.c`
- `tests/test_ui_state.c`
- `docs/00_INDEX.md`
- `docs/01_AGENT_DIRECTIVES.md`
- `docs/08_UI_UX.md`
- `docs/16_TUTORIAL.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/UX-002-ui-pan-guard-and-tutorial-directive.md`
- `sessions/2026-05-31/005-ui-pan-guard-and-tutorial-directive.md`

## Decisions

- Added `game_ui_point_allows_camera_pan` to keep the pan region defined by headless UI layout rather than SDL-only event code.
- Camera edge panning and right-drag panning now require the pointer to be over the world playfield.
- Top bar, bottom panel, right inspector, and event strip are treated as UI chrome that blocks camera pan gestures.
- Added `docs/16_TUTORIAL.md` as the tutorial directive and current tutorial plan instead of implementing a full in-game tutorial in this small UX ticket.

## Tests

- `clang-format -i src/game/main.c src/ui/ui_state.c src/ui/ui_state.h tests/test_ui_state.c`
- `cmake --build build-debug --target sim_tests`
- `.\build-debug\Debug\sim_tests.exe` (`ALL TESTS PASSED`)
- `cmake --build build-debug --target sim_headless`

## Verification note

- The local `build-debug` configuration reports `HEADLESS BUILD: SDL app target not built`, so SDL event-path compilation was not available in this environment.

## Next tasks

- Implement in-game tutorial state and SDL presentation from `docs/16_TUTORIAL.md`.
- Add screenshot/pixel-level UI regression checks for tutorial prompts once presentation exists.
