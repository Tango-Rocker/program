# TICKET ID: SCHED-001-deterministic-scheduled-events

## Objective

Implement persistent scheduled events with explicit due ticks and stable ordering.

## Context

Delayed effects, buff expiration, construction completion, and replay require scheduled consequences to be visible simulation state rather than hidden callbacks.

## Sprint

Sprint 5 - world scale, scheduling, and persistence.

## Dependencies

- EVT-002-fixed-capacity-event-queue
- SIM-001-headless-sim-context-and-tick

## Acceptance criteria

- Scheduled entries store due tick, type, source id, payload, and stable sequence id.
- Dispatch order is deterministic by due tick then sequence id.
- Cancelled entries are explicit and do not dispatch.
- Scheduler exposes bounded capacity and overflow result codes.
- Headless tests cover insertion order, same-tick ordering, cancellation, overflow, and dispatch across tick advance.

## Allowed files

- `src/sim/scheduler.h`
- `src/sim/scheduler.c`
- `tests/test_scheduler.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/06_ECS_AND_EVENTS.md`
- `docs/12_TESTING.md`

## Out of scope

- File persistence.
- UI timeline display.
- Priority queues with dynamic allocation.
- Long-running calendar systems.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the scheduler test target or full `sim_tests` if tests remain single-binary.

## Notes

Do not use function pointers as persistent scheduled behavior. Store data facts and let systems interpret them.
