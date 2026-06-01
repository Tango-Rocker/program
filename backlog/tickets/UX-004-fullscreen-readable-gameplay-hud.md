# TICKET ID: UX-004-fullscreen-readable-gameplay-hud

## Status

Done.

## Goal

Make the SDL experience fullscreen and easier to read by shifting the HUD away from debug telemetry toward gameplay information.

## Allowed files

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

## Acceptance criteria

- SDL app requests fullscreen at startup and falls back to a resizable window if fullscreen creation fails.
- Main menu and action buttons are larger and more readable.
- Gameplay HUD prioritizes party position, horde pressure, selected target, terrain/noise, current order, latest alert, route summary, and action buttons.
- Debug-style seed/tick/event count/demo-chain summaries are removed from the primary gameplay HUD.
- Headless layout tests cover the larger gameplay HUD sizing.
