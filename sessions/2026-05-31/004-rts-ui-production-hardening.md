# Session 004 - RTS UI Production Hardening

## Goal

Implement the high-risk findings from the game-development/industry review of the current RTS UI/input slice.

## Files touched

- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/POLISH-006-rts-ui-production-hardening.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `src/game/default_scene.c`
- `src/game/main.c`
- `src/ui/sdl_hud.c`
- `src/ui/sdl_hud.h`
- `src/ui/ui_state.c`
- `src/ui/ui_state.h`
- `tests/test_default_scene.c`
- `tests/test_ui_state.c`

## Decisions

- Attack commands now require a materialized, alive horde actor; abstract horde markers alone are not valid combat targets.
- UI command surfaces no longer clear the event queue before staging/applying intent.
- Compact layout uses a two-row action grid and narrower forensic toggles so controls stay visible outside the inspector.
- Visible tile bounds are exposed in headless UI state and used by rendering/hit testing to avoid full-map scans.
- SDL path preview requests are debounced by party origin and target tile.
- SDL HUD text rendering now caches text textures by label/color and clears them on renderer changes or shutdown.
- SDL app camera panning/focus now routes through `GameCameraState` while keeping UI hex camera state as the projection input.
- HUD top-bar tick now displays authoritative scene tick instead of frame/app tick.

## Tests

- Added default-scene coverage for attack rejection before hostile materialization, successful attack after horde materialization, and event-queue preservation across UI command entry points.
- Added UI layout coverage for compact action/control containment and visible tile bounds being smaller than the full map.
- Passed `cmake --build cmake-build-debug --target sim_tests` with MSYS UCRT on `PATH`.
- Passed `.\cmake-build-debug\sim_tests.exe`.
- Passed `cmake --build cmake-build-debug --target sim_app` with MSYS UCRT on `PATH`.
- Passed `.\scripts\verify-debug.ps1`.
- Passed `ctest --test-dir cmake-build-debug --output-on-failure`.
- Passed `git diff --check`.
- Passed `sim_app` launch smoke; the app started and was closed after the smoke interval.
- After formatting, forced a stale `libsim_core.a` rebuild and reran the checks above against the rebuilt archive.

## Next tasks

- Add screenshot/pixel-level UI regression checks for compact and desktop SDL layouts.
- Move minimap/world rendering toward chunked draw lists when map sizes grow beyond the current showcase dimensions.
- Promote combat cooldown/range presentation once tactical combat moves beyond the demo attack.
