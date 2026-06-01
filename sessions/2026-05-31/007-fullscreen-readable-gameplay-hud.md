# Session 007 - Fullscreen Readable Gameplay HUD

## Goal

Open the SDL app fullscreen and revise the HUD to emphasize readable gameplay information instead of debug telemetry.

## Files touched

- `src/platform/sdl_app.c`
- `src/ui/ui_state.c`
- `src/ui/sdl_hud.c`
- `tests/test_ui_state.c`
- `docs/08_UI_UX.md`
- `docs/16_TUTORIAL.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/UX-004-fullscreen-readable-gameplay-hud.md`
- `sessions/2026-05-31/007-fullscreen-readable-gameplay-hud.md`

## Decisions

- SDL now requests fullscreen window creation, with a logged fallback to a resizable window.
- Increased HUD font size, panel heights, map hex size, menu button size, and action button size.
- Replaced the debug top bar with party and horde status.
- Replaced the dense inspector/demo-chain block with a target-focused gameplay panel.
- Kept the `Why`/`Noise Map` controls because they explain consequences without exposing raw debug metrics.

## Tests

- `clang-format -i src/platform/sdl_app.c src/ui/ui_state.c src/ui/sdl_hud.c tests/test_ui_state.c`
- `cmake --build build-debug --target sim_tests`
- `.\build-debug\Debug\sim_tests.exe` (`ALL TESTS PASSED`)
- `cmake --build build-debug --target sim_headless`
- `$env:PATH='C:\Users\Tango\tools\msys64\ucrt64\bin;' + $env:PATH; cmake --build cmake-build-debug --target sim_app`

## Verification note

- A parallel `sim_headless` build initially collided with `sim_tests` on MSVC PDB writes; rerunning it serially passed.

## Next tasks

- Add visual screenshot regression once SDL UI screenshot automation is available.
- Build a dedicated tutorial step system that points at the new readable HUD elements.
