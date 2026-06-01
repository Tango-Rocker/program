# TICKET ID: POLISH-004-known-issues-burn-down

## Objective

Burn down or explicitly defer known issues with ticket references.

## Context

Known issues should stay useful. Items that are fixed should be removed or marked resolved; deferred risks should point to future tickets.

## Sprint

Sprint 12 - release-readiness polish and enhancement pass.

## Acceptance criteria

- Review `docs/13_KNOWN_ISSUES.md` against implemented systems.
- Remove fixed issues or move them to a resolved/deferred format.
- Every remaining issue has impact, current mitigation, and next ticket/reference.
- Add tickets only if a real untracked issue remains.
- Session archive records decisions.

## Allowed files

- `docs/13_KNOWN_ISSUES.md`
- `backlog/todo.md`
- `backlog/sprints.md`
- `backlog/tickets/*.md`
- `sessions/**`

## Out of scope

- Fixing large implementation defects unless they are trivial and within ticket scope.
- Source changes unrelated to known issue cleanup.
- Generated files.

## Required checks

- No code checks required unless code changes are made.

## Notes

Do not hide risk by deleting issues without evidence. Defer explicitly when needed.
