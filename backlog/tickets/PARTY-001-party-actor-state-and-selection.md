# TICKET ID: PARTY-001-party-actor-state-and-selection

## Objective

Introduce party actor state and deterministic party selection.

## Context

The player primarily controls a party, but the party must be authoritative simulation state rather than UI-only selection state.

## Sprint

Sprint 6 - party actions and combat causal chain.

## Dependencies

- ECS-002-entity-registry-implementation
- SIM-001-headless-sim-context-and-tick

## Acceptance criteria

- Party actor state includes entity id, position, health, faction/team, and selectable flag.
- Selection state is stored outside UI widgets and changes only through command/system APIs.
- Invalid or stale entity handles cannot become selected.
- Selection changes emit traceable facts.
- Headless tests cover valid selection, stale handle rejection, multi-member ordering, and deterministic selection trace output.

## Allowed files

- `src/sim/party.h`
- `src/sim/party.c`
- `tests/test_party.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/02_ARCHITECTURE_MAP.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`

## Out of scope

- UI selection boxes.
- Character sheets.
- Equipment.
- Movement path consumption.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the party test target or full `sim_tests` if tests remain single-binary.

## Notes

UI may request selection, but the simulation owns authoritative selected actor state.
