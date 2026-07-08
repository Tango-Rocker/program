# Session 002 - Stored World Manual Regeneration

## Goal

Keep the generated 40x map stored for the running app session and regenerate it only when the player explicitly chooses a main-menu action.

## Files touched

- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/UI-009-stored-world-manual-regeneration.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `docs/16_TUTORIAL.md`
- `src/game/main.c`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.[ch]`
- `tests/test_ui_state.c`

## Decisions

- Kept storage in memory for the app session; persistent save slots remain out of scope.
- Changed main-menu `New Game` presentation to `Enter World`, which reuses the current scene instead of reinitializing it.
- Added a dedicated `Regenerate` menu action that calls the existing scene load path and stays on the main menu.
- Kept UI state non-authoritative: the menu reports a regenerate hit, and the app bootstrap performs scene regeneration.

## Tests

- Added headless UI coverage for regenerate menu hit testing and label availability.

## Next tasks

- Add persistent save/load UI when core save slots are ready.
