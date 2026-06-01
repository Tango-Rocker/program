# Session 023 - Large Showcase Map

## Goal

Create a bigger default map that better showcases how simulation systems work across the world.

## Files touched

- `README.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/DEMO-002-large-showcase-map.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `src/game/default_scene.c`
- `src/platform/sdl_app.c`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.c`
- `tests/test_default_scene.c`
- `tests/test_ui_state.c`

## Decisions

- Expanded the default scene to an 18x12 seeded scenario, giving the demo 216 tiles.
- Derived noise and sensory field extents from the generated scenario bounds instead of keeping the compact 9x9 field.
- Increased demo combat range/projectile speed so the existing combat beat still crosses the larger map deterministically.
- Scaled HUD hex geometry down while keeping marker-priority hit testing and command controls intact.
- Used SDL window logical size for HUD layout/input so high-DPI renderer output does not push the inspector offscreen.
- Fixed field overlay reads to match q-major tile field storage on the larger field.

## Tests

- Added default-scene assertions for large map dimensions and tile count.
- Added UI layout assertions that the large horde anchor fits before the inspector and above the action panel.
- Passed `.\scripts\verify-debug.ps1`.
- Passed `cmake --build cmake-build-debug --target sim_app`.
- Passed `.\cmake-build-debug\sim_tests.exe`.
- Passed `ctest --test-dir cmake-build-debug --output-on-failure`.
- Captured `artifacts/large-showcase-map-smoke.png` from a visible `sim_app` launch.

## Next tasks

- Visual pass for density and readability on the larger map.
