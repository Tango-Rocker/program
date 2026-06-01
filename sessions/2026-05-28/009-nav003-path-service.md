# Session 009 - Complete NAV-003 path request service

## Goal

- Continue objective: complete all remaining sprints.
- Focus ticket: `NAV-003-path-request-service-budgeting`.

## Files touched

- `src/nav/path_service.c`
- `src/nav/path_service.h` (unchanged API)
- `tests/test_path_service.c`
- `CMakeLists.txt`
- `tests/test_main.c`
- `docs/07_NAVIGATION.md`
- `docs/12_TESTING.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/todo.md`

## Decisions and reasoning

- Kept `NAV-003` as a synchronous, deterministic service over `game_pathfind_query`.
- Implemented explicit per-slot handle/version lifecycle with states `PENDING`, `RESOLVED`, `FAILED`, `CANCELLED`, and `EXPIRED`.
- Added deterministic tick-budget scheduling in `game_path_service_advance_tick` using slot order and request budget.
- Added terminal-state TTL handling and slot cleanup so stale handles become invalid after slot reuse.
- Added headless test coverage for:
  - submit/status round-trip
  - cancellation behavior
  - expiry visibility and stale-handle cleanup
  - budget-limited stepping
  - handle version progression on slot reuse

## Tests

- Added `test_path_service` and wired it into `tests/test_main.c` and `CMakeLists.txt`.
- Did not execute build/tests in this turn per current interaction constraints.

## Next tasks

- Begin `COLONY-002-job-board-storage-and-reservations`.
- Then `COLONY-003-worker-job-selection-headless-sim`.
