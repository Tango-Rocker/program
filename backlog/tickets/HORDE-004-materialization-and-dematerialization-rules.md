# TICKET ID: HORDE-004-materialization-and-dematerialization-rules

## Objective

Add deterministic rules for materializing and dematerializing horde actors from abstract pressure.

## Context

The simulation needs to bridge offscreen pressure and local tactical actors without arbitrary spawning.

## Sprint

Sprint 7 - sensory ecology and horde escalation.

## Dependencies

- HORDE-003-abstract-pressure-group-lod
- ECS-002-entity-registry-implementation

## Acceptance criteria

- Materialization requires explicit region, pressure threshold, visibility/proximity input, and spawn budget.
- Spawned actor count and positions are deterministic for the same group state and seed.
- Dematerialization returns eligible actors to abstract pressure through documented rules.
- Transitions emit trace facts linking group ids and actor ids.
- Headless tests cover threshold spawn, budget limits, deterministic position choice, dematerialization, and trace linking.

## Allowed files

- `src/sim/horde_materialization.h`
- `src/sim/horde_materialization.c`
- `tests/test_horde_materialization.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/10_HORDE_AI.md`
- `docs/14_SIMULATION_DOCTRINE.md`

## Out of scope

- Local attack AI.
- Animation.
- Spawning from procedural encounter tables.
- Player-facing warning UI.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the horde materialization test target or full `sim_tests` if tests remain single-binary.

## Notes

Materialization must be explainable after the fact through trace ids and pressure history.
