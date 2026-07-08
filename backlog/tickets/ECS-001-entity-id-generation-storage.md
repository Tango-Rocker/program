# TICKET ID: ECS-001-entity-id-generation-storage

## Objective

Design ECS entity IDs with generation counters and baseline storage strategy.

## Context

Foundation-only ticket for future ECS core.

## Acceptance criteria

- draft schema for entity identifiers documented.
- storage map strategy planned.

## Allowed files

- `docs/03_MODULE_MAP.md`
- `docs/06_ECS_AND_EVENTS.md`

## Out of scope

- full ECS implementation.

## Required checks

- no code changes required in milestone 0.

## Notes

Keep deterministic recreation requirements in mind.
