# TICKET ID: ECON-001-stockpile-and-inventory-storage

## Objective

Implement deterministic inventory and stockpile storage primitives.

## Context

Colony logistics need explicit resource state before hauling and construction jobs can be meaningful.

## Sprint

Sprint 8 - colony logistics and construction.

## Dependencies

- ECS-002-entity-registry-implementation
- WORLD-004-chunked-world-map-storage

## Acceptance criteria

- Inventory storage tracks bounded item/resource stacks by owner entity or stockpile id.
- Add/remove/reserve operations return explicit result codes.
- Stockpile queries iterate in deterministic order.
- Reservations prevent duplicate consumption of the same resource quantity.
- Headless tests cover add/remove, insufficient resources, reservation conflict, release, and stable query ordering.

## Allowed files

- `src/colony/inventory.h`
- `src/colony/inventory.c`
- `tests/test_inventory.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/09_COLONY_JOBS.md`
- `docs/12_TESTING.md`

## Out of scope

- Item durability.
- UI inventory panels.
- Trading/economy prices.
- Content schema for item definitions.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the inventory test target or full `sim_tests` if tests remain single-binary.

## Notes

Use stable ids and bounded capacities. Avoid string-keyed hot-loop lookups in core simulation.
