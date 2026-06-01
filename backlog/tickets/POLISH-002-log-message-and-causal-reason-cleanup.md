# TICKET ID: POLISH-002-log-message-and-causal-reason-cleanup

## Objective

Clean up logs, trace reason codes, and causal report wording.

## Context

The world should be opaque but traceable. Logs and reason codes need to be consistent enough for developers and future player-facing reports.

## Sprint

Sprint 12 - release-readiness polish and enhancement pass.

## Dependencies

- UI-004-after-action-causal-report
- EVT-003-serializable-event-trace-log

## Acceptance criteria

- Reason codes use stable enums or ids where core systems need deterministic output.
- Human-readable text is centralized or documented where present.
- Causal reports avoid ambiguous or misleading labels.
- Tests cover stable output for representative logs/reports.
- Docs describe which logs are developer-facing versus future player-facing.

## Allowed files

- `src/core/log.*`
- `src/event/**`
- `src/sim/**`
- `src/ui/causal_report.*`
- `tests/**/*.c`
- `docs/14_SIMULATION_DOCTRINE.md`

## Out of scope

- Localization.
- Natural language generation.
- UI redesign.
- Large event schema migration.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run targeted event/log/report tests.

## Notes

Keep deterministic trace output stable. Cosmetic text should not break replay unless intentionally part of the fixture.
