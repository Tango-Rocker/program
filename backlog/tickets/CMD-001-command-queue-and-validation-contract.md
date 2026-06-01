# TICKET ID: CMD-001-command-queue-and-validation-contract

## Objective

Implement a small command queue and validation contract for simulation intent.

## Context

Commands are requests, not facts. This ticket provides the first command boundary so UI, AI, and replay can submit intent without mutating simulation state directly.

## Sprint

Sprint 2 - first causal world-hears-you chain.

## Dependencies

- SIM-001-headless-sim-context-and-tick
- EVT-002-fixed-capacity-event-queue

## Acceptance criteria

- Command envelope includes command type, requested tick, source id, and bounded payload.
- Queue operations are deterministic and fixed-capacity.
- Validation rejects malformed source ids, unsupported types, and commands outside the accepted tick window.
- Rejected commands return explicit results and do not mutate simulation state.
- Headless tests cover FIFO ordering, overflow, valid command acceptance, and invalid command rejection.

## Allowed files

- `src/sim/command.h`
- `src/sim/command.c`
- `tests/test_command_queue.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/06_ECS_AND_EVENTS.md`
- `docs/12_TESTING.md`

## Out of scope

- UI input binding.
- Network input.
- Full ability/action catalog.
- Command persistence beyond the in-memory queue.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the command queue test target or full `sim_tests` if tests remain single-binary.

## Notes

Keep command validation separate from event publication. Events should describe accepted consequences after authoritative mutation.
