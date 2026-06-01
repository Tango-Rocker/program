# TICKET ID: PERF-001-hot-loop-allocation-audit

## Objective

Audit hot-loop systems for hidden allocation and document remaining allocation behavior.

## Context

The project doctrine requires deterministic simulation and no undocumented hot-loop allocation. This pass finds and fixes obvious violations.

## Sprint

Sprint 11 - polish and architecture rework pass.

## Acceptance criteria

- Review simulation tick, field update, pathfinding, horde, colony, projectile, and scheduler loops.
- Remove avoidable dynamic allocations from hot loops or replace them with caller-owned scratch storage.
- Document any remaining allocation behavior and why it is acceptable.
- Add targeted tests or counters where practical to guard against regression.
- Update known issues if a larger performance problem is deferred.

## Allowed files

- `src/sim/**/*.c`
- `src/world/**/*.c`
- `src/nav/**/*.c`
- `src/colony/**/*.c`
- `tests/**/*.c`
- `docs/13_KNOWN_ISSUES.md`
- `docs/12_TESTING.md`

## Out of scope

- Full profiler integration.
- Algorithm rewrites not needed to remove allocation.
- Micro-optimizing cold paths.
- Renderer performance.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run targeted tests for changed modules.

## Notes

Prefer measured or obvious allocation fixes. Do not trade correctness for premature performance.
