# Session 001 - RTS Input Polish

## Goal

Review the current interactive large-world UI and polish it toward RTS game standards from the input/UI perspective.

## Files touched

- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/PLAYER-002-rts-input-polish.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `src/game/main.c`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.c`
- `src/ui/ui_state.h`
- `tests/test_ui_state.c`

## Review findings addressed

- Movement was button-only and did not support the RTS convention of right-click movement.
- Action buttons executed immediately against the prior selection instead of supporting target staging.
- Camera panning was tied to raw right-drag deltas, could leave map bounds, and lacked keyboard/edge movement.
- Pointer hit testing scanned the full 180x120 map even though only a small camera window is visible.
- The HUD did not expose staged action/hover feedback clearly.

## Decisions

- Added `GameUiActionMode` for non-authoritative staged actions.
- Right-clicking a world tile now submits a move order through `game_default_scene_move_party_to`.
- Clicking `Move`, `Interact`, or `Noise` stages that action; clicking a world tile applies the staged action.
- Added keyboard and edge camera panning with map-bound clamping.
- Bounded tile hit testing to the camera-visible range.
- Highlighted hover/target tiles and active action buttons in the SDL HUD.

## Tests

- Extended UI state tests for staged `Move`, `Interact`, and `Noise` actions.
- Passed `.\scripts\verify-debug.ps1`.
- Passed `cmake --build cmake-build-debug --target sim_tests`.
- Passed `.\cmake-build-debug\sim_tests.exe`.
- Passed `cmake --build cmake-build-debug --target sim_app`.
- Passed `ctest --test-dir cmake-build-debug --output-on-failure`.
- Passed `git diff --check`.

## Next tasks

- Add path preview and path-follow movement using the existing navigation service.
- Add minimap camera navigation for the 180x120 map.
- Add selection groups once multiple controllable party actors exist.
