# Session 021 - Real SDL UI HUD

## Goal

Implement the first real player-facing SDL HUD around the default scene, with non-authoritative UI state and one actionable `Emit Noise` command.

## Files touched

- `CMakeLists.txt`
- `backlog/active.md`
- `backlog/tickets/UI-005-real-sdl-player-hud.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `docs/13_KNOWN_ISSUES.md`
- `src/game/default_scene.[ch]`
- `src/game/main.c`
- `src/platform/app.h`
- `src/platform/sdl_app.c`
- `src/ui/ui_state.[ch]`
- `src/ui/sdl_hud.[ch]`
- `tests/test_default_scene.c`
- `tests/test_main.c`
- `tests/test_ui_state.c`

## Decisions

- Kept UI state headless and non-authoritative: selection, hover, panel toggles, command feedback, and layout/hit-tests live in `src/ui/ui_state`.
- Kept SDL and SDL_ttf isolated to the `sim_app` target through `src/ui/sdl_hud`.
- Routed `Emit Noise` through `GameCommandInspector`, the command queue, noise field impulse, horde attention, event log, and causal report.
- Retained shape-only rendering if Windows UI fonts cannot be loaded, with a warning.
- Replaced the temporary rectangular scene drawing with hex geometry and HUD panels.

## Tests

- Built `sim_app`.
- Built and ran `sim_tests`.
- Passed `.\scripts\verify-debug.ps1`.
- Passed `cmake --build cmake-build-debug --target sim_app`.
- Passed `.\cmake-build-debug\sim_tests.exe`.
- Passed `ctest --test-dir cmake-build-debug --output-on-failure`.
- Captured `artifacts/real-sdl-ui-smoke.png` from a visible `sim_app` launch.

## Next tasks

- Manual visual pass of `sim_app.exe` for readability and interaction feel.
- Add richer command affordances after the next product ticket defines scope.
