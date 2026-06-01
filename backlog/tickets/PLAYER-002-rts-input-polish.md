# TICKET ID: PLAYER-002-rts-input-polish

# Title
RTS input and targeting polish for the SDL HUD

# Status
Done

# Context
The first interactive large-world pass made movement and interaction possible, but the controls were still closer to a debug panel than a conventional RTS interface. Camera movement, target commands, and action feedback needed a small focused polish pass.

# Acceptance Criteria

- Right-clicking a world tile issues a move command through the scene command boundary.
- Action buttons stage target modes for move, interact, and noise commands.
- Clicking a world tile while an action is staged applies that action to the target tile.
- Keyboard and edge panning move the camera with map-bound clamping.
- UI hit testing avoids scanning the full 180x120 map on every pointer event.
- HUD feedback shows staged action state and hover targeting state without direct simulation mutation.

# Allowed Files

- `src/game/main.c`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.[ch]`
- `tests/test_ui_state.c`
- docs/backlog/session files

# Notes

This ticket keeps party movement immediate. Path preview, formation movement, minimap navigation, and command queues with delayed movement execution remain future work.
