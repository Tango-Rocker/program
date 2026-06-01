# TICKET ID: TEST-001-determinism-fuzz-smoke-suite

## Objective

Add a deterministic smoke suite over multiple seeds and command streams.

## Context

Golden tests catch known fixtures; fuzz-like smoke tests catch broad determinism regressions across small generated scenarios.

## Sprint

Sprint 11 - polish and architecture rework pass.

## Dependencies

- REPLAY-002-golden-scenario-replay-fixtures
- PROC-001-seeded-scenario-map-generator

## Acceptance criteria

- Test suite runs a bounded list of fixed seeds and generated command streams.
- Each case runs twice and compares trace/state summaries.
- Failure output identifies seed, command stream id, tick, and first mismatch.
- Runtime remains reasonable for local targeted testing.
- Docs explain when to run the smoke suite versus quick unit tests.

## Allowed files

- `tests/test_determinism_smoke.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/12_TESTING.md`

## Out of scope

- Random unbounded fuzzing.
- External fuzzing frameworks.
- Performance benchmarks.
- Long campaign simulation.

## Required checks

- Build with tests enabled.
- Run the determinism smoke suite.

## Notes

Seeds must be fixed and checked in. This is not a flaky random test.
