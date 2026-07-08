# TICKET ID: UI-009-stored-world-manual-regeneration

# Title
Stored world and manual regeneration menu action

# Status
Done

# Context
The 40x default map is expensive enough that it should not be regenerated implicitly every time the player enters gameplay from the main menu.

# Acceptance Criteria

- The SDL bootstrap generates the default scene once during app initialization.
- Returning to the main menu and choosing `Enter World` reuses the stored scene instead of calling scenario generation again.
- The main menu exposes a separate `Regenerate` action that explicitly rebuilds the default scene.
- UI hit testing covers the regenerate menu target headlessly.
- The change preserves UI-to-simulation boundaries; menu input selects app actions and does not mutate world storage directly.

# Allowed Files

- `src/game/main.c`
- `src/ui/**`
- `tests/test_ui_state.c`
- docs/backlog/session files

# Notes

This is in-memory storage for the running app session. Persistent save slots remain out of scope.
