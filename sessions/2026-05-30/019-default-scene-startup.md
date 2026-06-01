# Session: default scene startup

## Goal
- Build a deterministic default scene and make it the app startup path.

## Files touched
- `src/game/default_scene.[ch]`
- `src/game/main.c`
- `tests/test_default_scene.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `scripts/build-debug.ps1`
- `README.md`
- `docs/12_TESTING.md`
- `docs/14_SIMULATION_DOCTRINE.md`
- `tools/indexers/module_index.py`
- `generated/module_index.md`

## Decisions
- Added `GameDefaultScene` as owned runtime state, not global mutable state.
- The default scene runs a single opening chain: party noise command, field impulse, horde attention reaction, causal report, and worker job selection audit.
- Added `sim_headless` so the default app path builds and runs even when SDL3 is unavailable.
- Left SDL rendering optional; when SDL is available the existing debug overlay receives the scene event log.

## Validation
- `scripts/verify-debug.ps1` passes and now builds both `sim_tests` and `sim_headless`.
- `cmake-build-debug/sim_tests.exe` passes all tests.
- `cmake-build-debug/sim_headless.exe` runs the default scene and logs `default_scene seed=1337 events=3 pressure=14 posture=2 worker_audits=1`.
- `ctest --test-dir cmake-build-debug --output-on-failure` passes.
- `scripts/generate-indexes.ps1` and `git diff --check` pass.

## Next tasks
- Add an SDL visual pass for the generated map if SDL3 is available in the target environment.
- Consider making `sim_headless` a shorter smoke run if it becomes too slow for CI.
