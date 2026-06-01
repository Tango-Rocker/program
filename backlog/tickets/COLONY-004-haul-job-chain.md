# TICKET ID: COLONY-004-haul-job-chain

## Objective

Implement a deterministic haul job chain from stockpile reservation to delivery.

## Context

Workers need to move resources through explicit job states rather than abstractly completing work orders.

## Sprint

Sprint 8 - colony logistics and construction.

## Dependencies

- COLONY-002-job-board-storage-and-reservations
- ECON-001-stockpile-and-inventory-storage
- NAV-003-path-request-service-budgeting

## Acceptance criteria

- Haul jobs reserve source resource, worker capacity, destination, and route request handle.
- Job lifecycle covers reserve, pickup, in-transit, delivery, stalled, and aborted states.
- Failed path or missing resource transitions leave audit facts.
- Delivery mutates inventory through inventory APIs only.
- Headless tests cover successful haul, resource conflict, path failure, abort cleanup, and audit trace output.

## Allowed files

- `src/colony/haul_job.h`
- `src/colony/haul_job.c`
- `tests/test_haul_job.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/09_COLONY_JOBS.md`
- `docs/12_TESTING.md`

## Out of scope

- Animation.
- Complex carrying equipment.
- Multi-worker hauling.
- UI stockpile controls.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the haul job test target or full `sim_tests` if tests remain single-binary.

## Notes

Every reservation acquired by this chain must have an explicit release path.
