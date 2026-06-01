# TICKET ID: FIELD-001-multi-field-registry

## Objective

Generalize scalar tile fields into a deterministic multi-field registry.

## Context

Noise exists as the first field. The world also needs scent, light, blood, heat, and other signals without each system inventing separate storage semantics.

## Sprint

Sprint 7 - sensory ecology and horde escalation.

## Dependencies

- WORLD-003-tile-field-storage

## Acceptance criteria

- Registry stores a bounded set of named or enum field families.
- Field lookup/update uses explicit field ids and result codes.
- Field iteration order is deterministic.
- Missing or disabled field ids fail closed.
- Headless tests cover multiple fields on the same tile, independent decay, disabled field access, and stable iteration order.

## Allowed files

- `src/world/field_registry.h`
- `src/world/field_registry.c`
- `tests/test_field_registry.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/14_SIMULATION_DOCTRINE.md`
- `docs/12_TESTING.md`

## Out of scope

- Rendering overlays.
- Domain-specific light/scent/blood rules.
- Infinite field streaming.
- Save/load beyond in-memory state.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the field registry test target or full `sim_tests` if tests remain single-binary.

## Notes

The first registry should reduce duplication, not become a generic untyped bag of mutable global state.
