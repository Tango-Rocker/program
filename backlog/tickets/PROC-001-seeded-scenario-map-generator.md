# TICKET ID: PROC-001-seeded-scenario-map-generator

## Objective

Implement a deterministic seeded scenario map generator for test fixtures.

## Context

Golden replay tests need stable initial worlds with topology, fields, jobs, and actors without hand-writing every tile.

## Sprint

Sprint 10 - data, scenarios, and golden replay coverage.

## Dependencies

- WORLD-004-chunked-world-map-storage
- WORLD-005-region-topology-index
- ECON-001-stockpile-and-inventory-storage

## Acceptance criteria

- Generator accepts explicit seed and scenario config.
- Output includes world tiles, basic passability, initial party/horde/colony anchors, and optional stockpiles.
- Same seed/config produces stable generated output.
- Generated maps pass topology validation.
- Headless tests cover stable output, different seed difference, invalid config rejection, and topology sanity.

## Allowed files

- `src/world/scenario_gen.h`
- `src/world/scenario_gen.c`
- `tests/test_scenario_gen.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/12_TESTING.md`
- `docs/14_SIMULATION_DOCTRINE.md`

## Out of scope

- Production terrain generation.
- Biomes.
- Runtime map editor.
- Asset placement.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the scenario generator test target or full `sim_tests` if tests remain single-binary.

## Notes

This is a test/scenario generator first. Keep output intentionally small and inspectable.
