# TICKET ID: SAVE-001-core-snapshot-format

## Objective

Define and implement a minimal core snapshot format for deterministic state round-trip tests.

## Context

Replay proves command determinism, but save/load requires state serialization. This ticket starts with enough state to round-trip the current simulation slice.

## Sprint

Sprint 5 - world scale, scheduling, and persistence.

## Dependencies

- SIM-001-headless-sim-context-and-tick
- WORLD-004-chunked-world-map-storage
- SCHED-001-deterministic-scheduled-events

## Acceptance criteria

- Snapshot contains version, seed/RNG state, current tick, entity registry state, scheduler state, and selected world map state.
- Load rejects unsupported versions and malformed input with explicit errors.
- Round-trip load/save produces stable output for the same state.
- Snapshot tests verify deterministic continuation after load.
- Format is documented as provisional and intentionally small.

## Allowed files

- `src/sim/snapshot.h`
- `src/sim/snapshot.c`
- `tests/test_snapshot.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/12_TESTING.md`
- `docs/14_SIMULATION_DOCTRINE.md`

## Out of scope

- Full content database serialization.
- Binary compression.
- Backward compatibility beyond version rejection.
- User-facing save UI.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the snapshot test target or full `sim_tests` if tests remain single-binary.

## Notes

Use stable deterministic text or binary output. Do not include wall-clock time, file paths, or pointer addresses.
