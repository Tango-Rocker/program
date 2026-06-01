# TICKET ID: COLONY-003-worker-job-selection-headless-sim

## Objective

Implement deterministic worker job selection from the job board.

## Context

The colony should begin acting autonomously in headless tests. This ticket adds a narrow worker decision system without inventory, animation, or full schedules.

## Sprint

Sprint 3 - autonomous work and pathing.

## Dependencies

- COLONY-002-job-board-storage-and-reservations
- EVT-003-serializable-event-trace-log

## Acceptance criteria

- Worker state includes role, stamina, current job id, and threat posture or threat input.
- Selection ranks available jobs deterministically by role match, urgency, distance/cost placeholder, and stable id tie-break.
- Threat input can stall routine work or prefer emergency jobs.
- Successful selection reserves a job through the job board API.
- Decision emits an audit event or trace entry with the selected reason code.
- Headless tests cover role match, tie-break, stale reservation avoidance, threat preemption, and audit trace output.

## Allowed files

- `src/colony/worker_ai.h`
- `src/colony/worker_ai.c`
- `tests/test_worker_jobs.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/09_COLONY_JOBS.md`
- `docs/14_SIMULATION_DOCTRINE.md`
- `docs/12_TESTING.md`

## Out of scope

- Real movement/path consumption.
- Resource hauling.
- Full daily schedules.
- UI worker panel.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the worker job test target or full `sim_tests` if tests remain single-binary.

## Notes

Prefer explicit reason enums over free-form strings in core simulation data.
