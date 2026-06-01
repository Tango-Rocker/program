# TICKET ID: FIELD-002-light-scent-blood-field-updates

## Objective

Implement deterministic light, scent, and blood field update rules.

## Context

Multiple fields make the world feel reactive. This ticket adds three simple field families with distinct decay/impulse behavior and traceable sources.

## Sprint

Sprint 7 - sensory ecology and horde escalation.

## Dependencies

- FIELD-001-multi-field-registry
- EVT-003-serializable-event-trace-log

## Acceptance criteria

- Light, scent, and blood fields each have documented impulse and decay rules.
- Updates use deterministic integer or fixed-point arithmetic.
- Field updates emit or append causal trace entries where appropriate.
- Systems can sample these fields without mutating them.
- Headless tests cover impulse application, decay, clamping, independent field behavior, and trace output.

## Allowed files

- `src/sim/sensory_fields.h`
- `src/sim/sensory_fields.c`
- `tests/test_sensory_fields.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/14_SIMULATION_DOCTRINE.md`
- `docs/12_TESTING.md`

## Out of scope

- Complex visibility/fog of war.
- Fluid propagation.
- Rendering overlays.
- AI use beyond field sampling.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the sensory fields test target or full `sim_tests` if tests remain single-binary.

## Notes

Keep each field rule intentionally small. The goal is a repeatable sensory ecology base, not final balance.
