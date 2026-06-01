# TICKET ID: REWORK-001-public-api-header-audit

## Objective

Audit public headers and reduce accidental API surface.

## Context

After ten implementation sprints, module boundaries should be tightened before more systems depend on accidental exports.

## Sprint

Sprint 11 - polish and architecture rework pass.

## Acceptance criteria

- Public headers expose only functions/types needed across modules.
- Internal helpers are moved to `.c` files or private headers where appropriate.
- Header comments document ownership, mutation, and deterministic assumptions for public APIs.
- Build includes all changed headers from a clean configure.
- Docs note any intentional public API boundary changes.

## Allowed files

- `src/**/*.h`
- `include/**/*.h`
- `docs/03_MODULE_MAP.md`
- `docs/04_CODING_STANDARD.md`
- `docs/15_AGENT_WORKFLOW.md`

## Out of scope

- Behavioral feature changes.
- Large file moves.
- Generated files.
- Style-only churn outside touched headers.

## Required checks

- Format changed C files if any C files are touched.
- Build with tests enabled.
- Run targeted tests for modules whose APIs changed.

## Notes

Do not broaden APIs for convenience. If a boundary feels awkward, document it and make the smallest safe rework.
