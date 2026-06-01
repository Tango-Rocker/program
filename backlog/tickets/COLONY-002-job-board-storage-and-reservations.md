# TICKET ID: COLONY-002-job-board-storage-and-reservations

## Objective

Implement a deterministic colony job board with work orders and reservations.

## Context

`COLONY-001` documented the job board contract. This ticket provides the shared storage that later worker AI can consume.

## Sprint

Sprint 3 - autonomous work and pathing.

## Dependencies

- ECS-002-entity-registry-implementation
- SIM-001-headless-sim-context-and-tick

## Acceptance criteria

- Job board supports creating work orders with stable ids, target, required role/resource flags, duration band, and state.
- Reservation API prevents duplicate claims for the same job.
- Stale reservations expire after a deterministic tick timeout.
- State transitions cover `OPEN`, `RESERVED`, `IN_PROGRESS`, `STALLED`, `DONE`, and `ABORTED`.
- Invalid transitions return explicit errors and leave board state unchanged.
- Headless tests cover creation, reservation conflict, expiration, valid transitions, and invalid transition rejection.

## Allowed files

- `src/colony/job_board.h`
- `src/colony/job_board.c`
- `tests/test_job_board.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/09_COLONY_JOBS.md`
- `docs/12_TESTING.md`

## Out of scope

- Worker choice AI.
- Inventory/resource transfer.
- Pathing to job targets.
- UI job panel.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the job board test target or full `sim_tests` if tests remain single-binary.

## Notes

The board is authoritative simulation state. UI must not mutate it directly.
