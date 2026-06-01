# TICKET ID: EVT-002-fixed-capacity-event-queue

## Objective

Implement a fixed-capacity typed event queue for deterministic headless simulation tests.

## Context

`EVT-001` documented event categories and ordering expectations. This ticket provides the smallest concrete queue needed before logs, noise, and horde systems can exchange facts.

## Sprint

Sprint 1 - deterministic simulation kernel.

## Acceptance criteria

- Event envelope includes type, tick, source id, and a small payload union or byte payload with documented limits.
- Queue preserves FIFO order for same-tick events.
- Push/pop/peek/clear operations have explicit success/failure results.
- Overflow fails closed and does not corrupt existing queued events.
- Headless tests cover ordering, empty pop, clear, and overflow.

## Allowed files

- `src/event/event.h`
- `src/event/event.c`
- `tests/test_event_queue.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/06_ECS_AND_EVENTS.md`
- `docs/12_TESTING.md`

## Out of scope

- Dynamic allocation.
- Subscriber callbacks.
- Persistent event logs.
- Replay serialization.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the event queue test target or full `sim_tests` if tests remain single-binary.

## Notes

Events are immutable facts after mutation. Do not use the queue as hidden control flow for state changes.
