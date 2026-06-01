# TICKET ID: WORLD-006-structure-footprints-and-blocking

## Objective

Add structure footprints and deterministic blocking updates to world storage.

## Context

Construction must affect pathing, topology, fields, and future rendering through explicit world APIs.

## Sprint

Sprint 8 - colony logistics and construction.

## Dependencies

- WORLD-004-chunked-world-map-storage
- WORLD-005-region-topology-index
- COLONY-005-construction-worksite-lifecycle

## Acceptance criteria

- Structure definitions include stable id, footprint tiles, passability/blocking flags, and field interaction flags.
- Placement validates all footprint tiles before mutation.
- Placement/removal updates world tile flags through explicit APIs.
- Topology dirtying/rebuild hook is documented or implemented minimally.
- Headless tests cover valid placement, blocked placement, removal, topology dirty flag, and deterministic footprint ordering.

## Allowed files

- `src/world/structure.h`
- `src/world/structure.c`
- `tests/test_structure.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/03_MODULE_MAP.md`
- `docs/07_NAVIGATION.md`

## Out of scope

- Structure art.
- Damage/destruction.
- Blueprint UI.
- Complex multi-level buildings.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the structure test target or full `sim_tests` if tests remain single-binary.

## Notes

Placement must be all-or-nothing. Partial footprint mutation is a correctness bug.
