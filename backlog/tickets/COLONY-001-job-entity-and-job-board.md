# TICKET ID: COLONY-001-job-entity-and-job-board

## Objective

Define colony worker and job-board model for autonomous simulation.

## Context

Workers should continue operating when player attention is elsewhere.

## Acceptance criteria

- job states, reservations, and worker threat response documented.
- autonomy rules for idle workers documented.

## Allowed files

- `docs/09_COLONY_JOBS.md`
- `docs/02_ARCHITECTURE_MAP.md`

## Out of scope

- full colony simulation runtime.

## Required checks

- model includes threat-state impacts.

## Notes

Job-board should support emergency override and stale work-order cleanup.
