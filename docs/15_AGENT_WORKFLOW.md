# 15_AGENT_WORKFLOW

## Session process

1. Open the ticket in `backlog/active.md`.
2. Read related docs and module boundaries.
3. Make the smallest change for this ticket.
4. Run targeted tests/builds.
5. Record results and artifacts in `sessions/YYYY-MM-DD/NNN-...`.

For Sprint 4 index maintenance, regenerate using:

- `scripts/generate_indexes.sh`
- `scripts/generate-indexes.ps1`

For the Sprint 10 status report only, regenerate using:

- `scripts/generate_sprint_status.sh`
- `scripts/generate-sprint-status.ps1`

## Suggested ticket lifecycle

- `todo` contains future work.
- `active` contains current focus and next task.
- `done` records completed milestones.
- `backlog/tickets` contains per-ticket acceptance criteria and scope.

## Handoff standards

- Include files touched.
- Include decisions and risks.
- Include explicit test results.
- Leave no ambiguous “later” items without a ticket reference.
