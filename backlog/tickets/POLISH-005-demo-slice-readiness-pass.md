# TICKET ID: POLISH-005-demo-slice-readiness-pass

## Objective

Prepare a small deterministic demo slice that can be built, tested, replayed, and explained.

## Context

After twelve sprints, the project should have a coherent engineering slice demonstrating the doctrine without pretending to be a full game.

## Sprint

Sprint 12 - release-readiness polish and enhancement pass.

## Dependencies

- REPLAY-002-golden-scenario-replay-fixtures
- UI-004-after-action-causal-report
- POLISH-003-build-script-and-ci-hardening

## Acceptance criteria

- Demo scenario includes party action, noise/field consequence, horde reaction, and at least one colony/autonomy effect where implemented.
- Replay fixture can reproduce the demo event trace.
- README or docs describe how to run the demo/test slice.
- Known missing production features are clearly marked as out of scope.
- Session archive summarizes final status and next recommended milestone.

## Allowed files

- `data/**`
- `tests/fixtures/**`
- `tests/**/*.c`
- `README.md`
- `docs/12_TESTING.md`
- `docs/14_SIMULATION_DOCTRINE.md`
- `backlog/active.md`
- `backlog/done.md`
- `sessions/**`

## Out of scope

- Shipping build packaging.
- Balancing.
- New production art/audio.
- Claims that the game is feature complete.

## Required checks

- Build with tests enabled.
- Run full headless test suite.
- Run demo replay fixture.

## Notes

This is a readiness pass for the current slice, not a release declaration.
