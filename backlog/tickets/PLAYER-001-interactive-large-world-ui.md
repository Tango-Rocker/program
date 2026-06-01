# TICKET ID: PLAYER-001-interactive-large-world-ui

# Title
Interactive large-world player controls

# Status
Done

# Context
The showcase HUD displayed the simulation, but the player still needed direct controls for moving the party and interacting with selected world tiles. The larger demo map also needed to scale from a compact showcase into a meaningfully large world without losing clickability.

# Acceptance Criteria

- The default scene map covers 100x the previous 18x12 area.
- The SDL HUD exposes player-facing `Move`, `Interact`, and `Noise` actions.
- Selection and marker-priority hit testing still work on a camera-centered map.
- Player movement and interaction are submitted through scene command surfaces, not direct UI mutation.
- Headless tests cover large map scale, movement, interaction, and UI button hit testing.

# Allowed Files

- `src/event/event.h`
- `src/game/default_scene.[ch]`
- `src/game/main.c`
- `src/sim/command.[ch]`
- `src/ui/sdl_hud.[ch]`
- `src/ui/ui_state.[ch]`
- `src/world/scenario_gen.c`
- `tests/test_default_scene.c`
- `tests/test_ui_state.c`
- docs/backlog/session files

# Notes

This pass does not implement full pathfinding movement, animation, minimap, inventory interactions, or production art. Movement is deterministic tile relocation through the default-scene command boundary.
