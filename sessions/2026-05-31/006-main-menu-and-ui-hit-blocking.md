# Session 006 - Main Menu and UI Hit Blocking

## Goal

Fix remaining UI hover fallthrough and add a minimal main menu with New Game, Load, Settings, and Tutorial screens.

## Files touched

- `src/game/main.c`
- `src/ui/ui_state.h`
- `src/ui/ui_state.c`
- `src/ui/sdl_hud.c`
- `tests/test_ui_state.c`
- `docs/08_UI_UX.md`
- `docs/16_TUTORIAL.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/UX-003-main-menu-and-ui-hit-blocking.md`
- `sessions/2026-05-31/006-main-menu-and-ui-hit-blocking.md`

## Decisions

- Added explicit `GAME_UI_HIT_CHROME` so empty UI panels no longer fall through to world hits.
- Added UI screen state and menu hit testing in `ui_state` so menu behavior is headless-testable.
- Kept `New Game` scene reset in `src/game/main.c`; UI only reports menu intent.
- Added first-pass SDL menu rendering without adding save-slot persistence or a full settings system.

## Tests

- `clang-format -i src/game/main.c src/ui/ui_state.c src/ui/ui_state.h src/ui/sdl_hud.c tests/test_ui_state.c`
- `cmake --build build-debug --target sim_tests`
- `.\build-debug\Debug\sim_tests.exe` (`ALL TESTS PASSED`)
- `cmake --build build-debug --target sim_headless`
- `$env:PATH='C:\Users\Tango\tools\msys64\ucrt64\bin;' + $env:PATH; cmake --build cmake-build-debug --target sim_app`

## Verification note

- The first parallel `sim_headless` build collided with `sim_tests` on MSVC PDB writes; rerunning it serially passed.
- The SDL-enabled build requires the MSYS2 UCRT bin directory on `PATH` so GCC can find its runtime DLLs.

## Next tasks

- Add persistent save-slot UI when save/load reaches player-facing scope.
- Replace the concise tutorial screen with dedicated tutorial step state and prompt eligibility tests.
