# TICKET ID: RENDER-002-hex-world-debug-renderer

## Objective

Add an optional SDL debug renderer for visible hex world tiles.

## Context

The simulation needs a basic visual surface for debugging map state while preserving headless testability.

## Sprint

Sprint 9 - player-facing debug and control surfaces.

## Dependencies

- WORLD-004-chunked-world-map-storage
- RENDER-001-camera-transform-and-party-leash

## Acceptance criteria

- Renderer compiles only when SDL target is available.
- Renderer reads immutable or snapshot world tile data.
- Headless builds with `GAME_ENABLE_SDL=OFF` continue to configure and build.
- Debug colors/flags distinguish blocked, open, selected, and field-highlighted tiles.
- Documentation states renderer is non-authoritative.

## Allowed files

- `src/render/hex_debug_render.h`
- `src/render/hex_debug_render.c`
- `src/platform/sdl_app.c`
- `CMakeLists.txt`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`

## Out of scope

- Production art.
- Sprite batching.
- Fog of war.
- UI interaction.

## Required checks

- Format changed C files.
- Build headless with `GAME_ENABLE_SDL=OFF`.
- If SDL3 is available, build the SDL app target.

## Notes

Rendering should consume snapshots or read-only views. It must not update world flags.
