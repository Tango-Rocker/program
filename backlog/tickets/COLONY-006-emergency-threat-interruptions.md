# TICKET ID: COLONY-006-emergency-threat-interruptions

## Objective

Implement threat-driven interruption rules for colony jobs.

## Context

Workers should react to horde/threat state with traceable decisions. This ticket adds emergency transitions without full panic simulation.

## Sprint

Sprint 8 - colony logistics and construction.

## Dependencies

- COLONY-003-worker-job-selection-headless-sim
- HORDE-002-attention-pressure-from-noise-field

## Acceptance criteria

- Threat input can stall routine jobs, release reservations, and create emergency assistance jobs.
- Interruption reasons are explicit enums and traceable.
- Emergency jobs rank above routine jobs under documented conditions.
- Stalled jobs can resume or abort deterministically when threat clears.
- Headless tests cover interruption, reservation release, emergency priority, resume, abort, and audit trace output.

## Allowed files

- `src/colony/emergency.h`
- `src/colony/emergency.c`
- `tests/test_colony_emergency.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/09_COLONY_JOBS.md`
- `docs/10_HORDE_AI.md`

## Out of scope

- Full morale/panic.
- Combat AI for workers.
- Shelter building behavior.
- UI alerts.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the colony emergency test target or full `sim_tests` if tests remain single-binary.

## Notes

Do not erase why work was interrupted. The audit trail is part of the product feel.
