# TICKET ID: UX-001-debug-overlay-legibility-pass

## Objective

Improve debug overlay legibility for event, field, and world inspection.

## Context

Debug overlays exist to help understand the simulation. They should be readable enough to support implementation and testing work.

## Sprint

Sprint 11 - polish and architecture rework pass.

## Dependencies

- UI-001-sdl-debug-event-log-overlay
- UI-003-field-overlay-debug-views
- RENDER-002-hex-world-debug-renderer

## Acceptance criteria

- Overlay uses consistent colors, labels, and visibility toggles for major debug layers.
- Event trace display can show recent event type/source/tick information where text rendering is available.
- Field overlays expose value buckets clearly enough to diagnose propagation.
- Headless builds remain unaffected.
- Docs note overlay limitations and usage.

## Allowed files

- `src/ui/debug_overlay.h`
- `src/ui/debug_overlay.c`
- `src/ui/field_overlay.c`
- `src/render/hex_debug_render.c`
- `src/platform/sdl_app.c`
- `docs/08_UI_UX.md`

## Out of scope

- Production UI skin.
- Full font/text rendering system if unavailable.
- Player-facing tutorial.
- Gameplay balancing.

## Required checks

- Format changed C files.
- Build headless with `GAME_ENABLE_SDL=OFF`.
- If SDL3 is available, build the SDL app target.

## Notes

This is a debug legibility pass, not a visual redesign.
