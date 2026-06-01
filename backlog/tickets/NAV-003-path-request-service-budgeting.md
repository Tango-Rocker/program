# TICKET ID: NAV-003-path-request-service-budgeting

## Objective

Implement deterministic path request handles with per-tick budget processing.

## Context

Path consumers should not directly own path calculation internals. This ticket turns the documented handle lifecycle into a small service over the path query primitive.

## Sprint

Sprint 3 - autonomous work and pathing.

## Dependencies

- NAV-002-hex-a-star-path-query
- SIM-001-headless-sim-context-and-tick

## Acceptance criteria

- Service supports submit, cancel, query status, and retrieve result operations.
- Handles expose `PENDING`, `RESOLVED`, `FAILED`, `CANCELLED`, and `EXPIRED` states.
- Per-tick processing consumes a deterministic node/request budget.
- TTL expiration is deterministic and logged or traceable.
- Result versioning prevents consumers from accepting stale resolved data.
- Headless tests cover lifecycle states, cancellation, expiration, budgeting, and result version checks.

## Allowed files

- `src/nav/path_service.h`
- `src/nav/path_service.c`
- `tests/test_path_service.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/07_NAVIGATION.md`
- `docs/12_TESTING.md`

## Out of scope

- Worker AI path consumption.
- Background threads.
- Path cache persistence.
- Render debug overlays.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the path service test target or full `sim_tests` if tests remain single-binary.

## Notes

Cancellation must fail closed. Keep cancelled accounting traceable for replay/debugging.
