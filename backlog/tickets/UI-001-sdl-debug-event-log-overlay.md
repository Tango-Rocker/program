# TICKET ID: UI-001-sdl-debug-event-log-overlay

## Objective

Add an optional SDL debug overlay that displays recent event trace entries without mutating simulation state.

## Context

The UI should provide forensic instruments. This ticket keeps the first UI/debug surface read-only and optional so headless simulation remains the primary test path.

## Sprint

Sprint 4 - replay, indexes, data boundary, and debug surface.

## Dependencies

- EVT-003-serializable-event-trace-log

## Acceptance criteria

- Overlay code compiles only when the SDL app target is enabled and available.
- Overlay reads a bounded snapshot of recent event trace entries.
- Overlay does not call simulation mutation APIs.
- Headless builds with `GAME_ENABLE_SDL=OFF` still configure and build.
- Documentation states the overlay is a debug/forensic surface, not authoritative state.

## Allowed files

- `src/ui/debug_overlay.h`
- `src/ui/debug_overlay.c`
- `src/platform/sdl_app.c`
- `CMakeLists.txt`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`

## Out of scope

- Interactive command controls.
- Full HUD layout.
- Minimap/noise meter.
- Audio integration.

## Required checks

- Format changed C files.
- Build headless with `GAME_ENABLE_SDL=OFF`.
- If SDL3 is available, build the SDL app target.

## Notes

This ticket must not make SDL required for tests or core simulation development.
