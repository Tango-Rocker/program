# TICKET ID: WORLD-001-axial-hex-coordinate-conversion

## Objective

Document axial/cube hex coordinate conversion and indexing plan.

## Context

Hex grid is the authoritative geometry backbone.

## Acceptance criteria

- coordinate conversion formulas are documented.
- indexing strategy for chunked storage outlined.

## Allowed files

- `docs/07_NAVIGATION.md`
- `docs/03_MODULE_MAP.md`

## Out of scope

- full nav implementation.

## Required checks

- unit tests planned for coordinate operations.

## Notes

Keep world coordinates deterministic and stable under serialization.
