# TICKET ID: UX-003-main-menu-and-ui-hit-blocking

## Status

Done.

## Goal

Finish the player-facing menu shell and fully prevent UI chrome from behaving like world input.

## Allowed files

- `src/game/main.c`
- `src/ui/ui_state.[ch]`
- `src/ui/sdl_hud.c`
- `tests/test_ui_state.c`
- `docs/08_UI_UX.md`
- `docs/16_TUTORIAL.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/UX-003-main-menu-and-ui-hit-blocking.md`
- `sessions/2026-05-31/006-main-menu-and-ui-hit-blocking.md`

## Acceptance criteria

- Empty UI chrome hit-tests as UI, not as world tiles or markers.
- Camera panning and game hotkeys are disabled while the app is in menu/tutorial/settings/load screens.
- SDL app starts on a main menu with `New Game`, `Load`, `Settings`, and `Tutorial`.
- `New Game` enters a reset default scene through app code, not through direct UI simulation mutation.
- Tutorial docs reflect the implemented first-pass tutorial menu.
- Headless UI tests cover menu hit testing and UI chrome blocking.
