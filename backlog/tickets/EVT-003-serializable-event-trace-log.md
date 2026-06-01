# TICKET ID: EVT-003-serializable-event-trace-log

## Objective

Add a deterministic event trace log for causal debugging.

## Context

The doctrine requires forensic traceability. This ticket creates a stable in-memory trace and serialization surface for event facts before replay consumes it.

## Sprint

Sprint 2 - first causal world-hears-you chain.

## Dependencies

- EVT-002-fixed-capacity-event-queue

## Acceptance criteria

- Event trace entries store tick, sequence number, event type, source id, and causal parent id or invalid parent.
- Appending preserves deterministic order and returns explicit overflow results.
- Serialization emits a stable text format suitable for tests and session debugging.
- Headless tests cover append order, causal parent recording, overflow, and stable serialization output.

## Allowed files

- `src/event/event_log.h`
- `src/event/event_log.c`
- `tests/test_event_log.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/06_ECS_AND_EVENTS.md`
- `docs/14_SIMULATION_DOCTRINE.md`
- `docs/12_TESTING.md`

## Out of scope

- Replay loading.
- Binary serialization.
- File I/O if an in-memory buffer is sufficient for tests.
- UI display.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the event log test target or full `sim_tests` if tests remain single-binary.

## Notes

The trace format must be stable enough for deterministic tests. Avoid timestamps from wall-clock time.
