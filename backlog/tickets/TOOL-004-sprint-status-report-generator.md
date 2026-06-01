# TICKET ID: TOOL-004-sprint-status-report-generator

## Objective

Generate a sprint status report from backlog files.

## Context

The backlog is now large enough that agents need a deterministic summary of todo/done/active state without manually scanning every ticket.

## Sprint

Sprint 10 - data, scenarios, and golden replay coverage.

## Dependencies

- TOOL-003-generated-module-index-prototype

## Acceptance criteria

- Tool reads `backlog/sprints.md`, `backlog/todo.md`, `backlog/done.md`, and ticket files.
- Output groups tickets by sprint with status: todo, active, done, missing, or duplicate.
- Report ordering is stable.
- Tool flags ticket ids listed in todo/done without matching files.
- Generated report is produced by the tool, not manually edited.

## Allowed files

- `tools/indexers/sprint_status.py`
- `scripts/generate_sprint_status.sh`
- `scripts/generate-sprint-status.ps1`
- `generated/sprint_status.md`
- `docs/15_AGENT_WORKFLOW.md`

## Out of scope

- Web dashboard.
- GitHub issue synchronization.
- Automatic ticket completion.
- Editing backlog files from the tool.

## Required checks

- Run the sprint status generator.
- Run it a second time and confirm output is stable.

## Notes

Generated files remain generated. Do not hand-edit `generated/sprint_status.md`.
