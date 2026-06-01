# Session 024 - Interactive Large World UI

## Goal

Build an interactive UI that lets the player move and interact with the world, while increasing the showcase map by 100x area.

## Files touched

- `README.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/PLAYER-001-interactive-large-world-ui.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `src/event/event.h`
- `src/game/default_scene.c`
- `src/game/default_scene.h`
- `src/game/main.c`
- `src/sim/command.c`
- `src/sim/command.h`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.c`
- `src/ui/ui_state.h`
- `src/world/scenario_gen.c`
- `tests/test_default_scene.c`
- `tests/test_ui_state.c`

## Decisions

- Scaled the default scenario from 18x12 to 180x120, preserving the 100x area requirement.
- Added `MOVE_PARTY` and `INTERACT_WORLD` command types for the default scene command boundary.
- Added authoritative `party_position` to the default scene so the HUD no longer treats the starting anchor as the current position.
- Kept movement deterministic and immediate for this pass; full pathfinding/animation remains future work.
- Added a camera-centered UI projection so the huge map stays clickable without rendering all tiles onscreen at once.

## Tests

- Added default-scene tests for 21600+ tiles, movement command application, interaction command application, and event count changes.
- Added UI tests for camera-centered layout and `Move`, `Interact`, and `Noise` button hit testing.
- Passed `cmake --build cmake-build-debug --target sim_tests`.
- Passed `.\cmake-build-debug\sim_tests.exe`.
- Passed `cmake --build cmake-build-debug --target sim_app`.

## Next tasks

- Add path preview and path-follow movement using the existing navigation service.
- Add a minimap or camera jump controls for the 180x120 map.
