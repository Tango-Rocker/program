# TICKET ID: SIM-001-headless-sim-context-and-tick

## Objective

Create a minimal headless `SimContext` that owns deterministic tick advancement.

## Context

The current app loop exists, but simulation state is not yet centralized. This ticket creates a small simulation context that later systems can plug into without depending on SDL or UI state.

## Sprint

Sprint 1 - deterministic simulation kernel.

## Dependencies

- ECS-002-entity-registry-implementation
- EVT-002-fixed-capacity-event-queue

## Acceptance criteria

- `SimContext` stores current tick, seed/RNG state, entity registry, and event queue references or owned storage.
- Initialization requires explicit seed/config values.
- Tick advance increments exactly one simulation tick and runs a documented empty system phase.
- Same seed and same tick count produce identical observable state in tests.
- Headless tests cover init, tick increment, deterministic repeat, and invalid config handling.

## Allowed files

- `src/sim/sim_context.h`
- `src/sim/sim_context.c`
- `tests/test_sim_tick.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/02_ARCHITECTURE_MAP.md`
- `docs/12_TESTING.md`

## Out of scope

- Real gameplay systems.
- SDL/app integration.
- Save/load.
- Multithreading.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the simulation tick test target or full `sim_tests` if tests remain single-binary.

## Notes

Do not introduce global simulation state. UI and platform layers must remain outside the authoritative simulation context.
