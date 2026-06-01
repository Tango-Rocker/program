# TICKET ID: HORDE-003-abstract-pressure-group-lod

## Objective

Implement LOD0 abstract horde pressure groups.

## Context

Hordes should exist offscreen as pressure before local actors are materialized. This ticket adds compact abstract group state that can react to fields and topology.

## Sprint

Sprint 7 - sensory ecology and horde escalation.

## Dependencies

- HORDE-002-attention-pressure-from-noise-field
- WORLD-005-region-topology-index
- FIELD-001-multi-field-registry

## Acceptance criteria

- Abstract group state includes region id, pressure, mass estimate, interest source, and posture.
- Group updates consume field samples and region adjacency deterministically.
- Pressure migration between neighboring regions follows stable ordering.
- State changes emit traceable facts.
- Headless tests cover pressure increase, decay, migration tie-breaks, posture change, and trace output.

## Allowed files

- `src/sim/horde_group.h`
- `src/sim/horde_group.c`
- `tests/test_horde_group.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/10_HORDE_AI.md`
- `docs/14_SIMULATION_DOCTRINE.md`

## Out of scope

- Local actor spawning.
- Combat behavior.
- Boss/tactical AI.
- Visual indicators.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the horde group test target or full `sim_tests` if tests remain single-binary.

## Notes

Keep LOD0 compact and serializable. Do not fake it with hidden spawned actors far offscreen.
