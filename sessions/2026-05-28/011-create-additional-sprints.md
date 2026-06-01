# Session: Create additional sprint backlog

## Goal

Create six additional feature/system-building sprints after the completed first four sprints, followed by two polish/rework/enhancement sprints.

## Scope

- Treat Sprints 1-4 as completed based on current backlog state.
- Update active/todo sprint tracking.
- Extend `backlog/sprints.md` from four sprints to twelve total sprints.
- Add scoped ticket files for Sprints 5-12.

## Files touched

- backlog/active.md
- backlog/todo.md
- backlog/sprints.md
- backlog/tickets/WORLD-004-chunked-world-map-storage.md
- backlog/tickets/WORLD-005-region-topology-index.md
- backlog/tickets/SCHED-001-deterministic-scheduled-events.md
- backlog/tickets/SAVE-001-core-snapshot-format.md
- backlog/tickets/PARTY-001-party-actor-state-and-selection.md
- backlog/tickets/COMBAT-001-ability-command-resolution.md
- backlog/tickets/COMBAT-002-projectile-lifecycle-and-impact-events.md
- backlog/tickets/BUFF-001-status-effect-lifecycle.md
- backlog/tickets/PARTICLE-001-particle-request-events.md
- backlog/tickets/FIELD-001-multi-field-registry.md
- backlog/tickets/FIELD-002-light-scent-blood-field-updates.md
- backlog/tickets/HORDE-003-abstract-pressure-group-lod.md
- backlog/tickets/HORDE-004-materialization-and-dematerialization-rules.md
- backlog/tickets/AUDIO-001-event-to-audio-request-bus.md
- backlog/tickets/ECON-001-stockpile-and-inventory-storage.md
- backlog/tickets/COLONY-004-haul-job-chain.md
- backlog/tickets/COLONY-005-construction-worksite-lifecycle.md
- backlog/tickets/COLONY-006-emergency-threat-interruptions.md
- backlog/tickets/WORLD-006-structure-footprints-and-blocking.md
- backlog/tickets/RENDER-001-camera-transform-and-party-leash.md
- backlog/tickets/RENDER-002-hex-world-debug-renderer.md
- backlog/tickets/UI-002-command-staging-and-inspector.md
- backlog/tickets/UI-003-field-overlay-debug-views.md
- backlog/tickets/UI-004-after-action-causal-report.md
- backlog/tickets/DATA-001-content-schema-and-validator.md
- backlog/tickets/LUA-002-content-driven-ability-and-job-prototypes.md
- backlog/tickets/PROC-001-seeded-scenario-map-generator.md
- backlog/tickets/REPLAY-002-golden-scenario-replay-fixtures.md
- backlog/tickets/TOOL-004-sprint-status-report-generator.md
- backlog/tickets/REWORK-001-public-api-header-audit.md
- backlog/tickets/PERF-001-hot-loop-allocation-audit.md
- backlog/tickets/TEST-001-determinism-fuzz-smoke-suite.md
- backlog/tickets/DOC-002-architecture-drift-review.md
- backlog/tickets/UX-001-debug-overlay-legibility-pass.md
- backlog/tickets/POLISH-001-error-handling-and-result-code-unification.md
- backlog/tickets/POLISH-002-log-message-and-causal-reason-cleanup.md
- backlog/tickets/POLISH-003-build-script-and-ci-hardening.md
- backlog/tickets/POLISH-004-known-issues-burn-down.md
- backlog/tickets/POLISH-005-demo-slice-readiness-pass.md
- sessions/2026-05-28/011-create-additional-sprints.md

## Decisions

- Set `WORLD-004-chunked-world-map-storage` as the next recommended ticket because scaled world storage is a prerequisite for later persistence, topology, construction, overlays, and scenarios.
- Kept Sprints 5-10 focused on feature/system growth: world scale, combat chain, sensory ecology, colony logistics, debug/control surfaces, and data/scenario coverage.
- Kept Sprints 11-12 focused on polish, rework, hardening, known issue burn-down, and demo slice readiness.
- Added ticket acceptance criteria and allowed files to preserve small-cut workflow.

## Tests

- Not run. This was a planning/backlog update only.

## Next tasks

- Start `WORLD-004-chunked-world-map-storage`.
- Keep implementation work scoped to one ticket at a time.
