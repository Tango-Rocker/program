# TICKET ID: REPLAY-002-golden-scenario-replay-fixtures

## Objective

Add golden replay fixtures for representative scenario slices.

## Context

As systems multiply, deterministic replay must catch drift in event traces and state transitions.

## Sprint

Sprint 10 - data, scenarios, and golden replay coverage.

## Dependencies

- REPLAY-001-deterministic-input-log-and-replay-shell
- PROC-001-seeded-scenario-map-generator
- COMBAT-002-projectile-lifecycle-and-impact-events
- COLONY-004-haul-job-chain

## Acceptance criteria

- At least three golden fixtures exist: combat/noise, colony haul, and horde pressure.
- Fixtures define seed, scenario config, command stream, tick count, and expected trace output.
- Test runner reports first differing trace entry on mismatch.
- Updating fixtures requires an explicit script or documented process.
- Tests cover fixture load, pass, and intentional mismatch reporting.

## Allowed files

- `tests/fixtures/replay/combat_noise.txt`
- `tests/fixtures/replay/colony_haul.txt`
- `tests/fixtures/replay/horde_pressure.txt`
- `src/sim/replay.c`
- `tests/test_replay_golden.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/12_TESTING.md`

## Out of scope

- Large campaign replay.
- Save/load compatibility testing.
- UI replay browser.
- Performance benchmarking.

## Required checks

- Build with tests enabled.
- Run the golden replay test target or full `sim_tests` if tests remain single-binary.

## Notes

Golden fixtures should be small enough to review in diffs.
