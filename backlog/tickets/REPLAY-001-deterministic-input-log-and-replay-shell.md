# TICKET ID: REPLAY-001-deterministic-input-log-and-replay-shell

## Objective

Create a minimal deterministic replay shell for seed, commands, ticks, and event output comparison.

## Context

Replay is required before broad automation grows. This ticket does not need full save/load; it only needs a small fixture proving same inputs produce same trace.

## Sprint

Sprint 4 - replay, indexes, data boundary, and debug surface.

## Dependencies

- CMD-001-command-queue-and-validation-contract
- EVT-003-serializable-event-trace-log
- SIM-001-headless-sim-context-and-tick

## Acceptance criteria

- Replay input stores seed, tick count, and an ordered command list.
- Replay runner applies commands at their requested ticks through the command queue.
- Runner captures deterministic event trace output for comparison.
- Same replay input produces byte-identical trace output in tests.
- Mismatched trace comparison reports the first differing line or entry.

## Allowed files

- `src/sim/replay.h`
- `src/sim/replay.c`
- `tests/test_replay.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/12_TESTING.md`
- `docs/14_SIMULATION_DOCTRINE.md`
- `docs/06_ECS_AND_EVENTS.md`

## Out of scope

- Full world save/load.
- Binary replay format.
- UI replay controls.
- Long-running performance benchmarks.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the replay test target or full `sim_tests` if tests remain single-binary.

## Notes

Do not bypass command validation during replay. Replay should exercise the same command path as live simulation.
