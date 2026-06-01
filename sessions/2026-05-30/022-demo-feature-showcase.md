# Session 022 - Demo Feature Showcase

## Goal

Implement the demo plan as a deterministic feature showcase visible from the default SDL scene.

## Files touched

- `README.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/DEMO-001-deterministic-feature-showcase.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `src/game/default_scene.[ch]`
- `src/game/main.c`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.c`
- `tests/test_default_scene.c`

## Decisions

- Kept the demo deterministic and headless-testable by making it part of `GameDefaultScene`, not an SDL-only script.
- Used existing subsystem APIs for each beat: sensory fields, horde group/materialization, emergency interruption, construction worksite startup, combat ability, projectile impact, status effect, audio request projection, particle request projection, and replay trace capture.
- Exposed compact demo summaries through `GameDefaultSceneDemoStatus`; the HUD reads this state but does not mutate simulation state.
- `Emit Noise` remains the player action and continues to submit through command inspection and the command queue.

## Tests

- Added default-scene assertions for each demo beat.
- Passed `.\scripts\verify-debug.ps1`.
- Passed `cmake --build cmake-build-debug --target sim_app`.
- Passed `.\cmake-build-debug\sim_tests.exe`.
- Passed `ctest --test-dir cmake-build-debug --output-on-failure`.
- Captured `artifacts/demo-feature-showcase-smoke.png` from a visible `sim_app` launch.

## Next tasks

- Visual pass for line fit and panel spacing in `sim_app`.
- A later presentation pass can add animation or richer art without changing the deterministic demo chain.
