# Session: Complete sprint 1 deterministic kernel

## Goal

Implement Sprint 1 and move core deterministic kernel tickets to done.

## Scope

- Implement ECS entity registry with generationed handles and deterministic free-list reuse.
- Implement fixed-capacity event queue with explicit result codes and overflow protection.
- Implement axial/cube hex helpers with deterministic chunk/local indexing for negative coordinates.
- Implement headless simulation context with deterministic tick advancement.
- Add/extend headless tests for each Sprint 1 feature.
- Update backlog/docs to mark Sprint 1 as complete and point backlog forward.

## Files touched

- CMakeLists.txt
- src/ecs/entity.h
- src/ecs/entity.c
- src/event/event.h
- src/event/event.c
- src/world/hex.h
- src/world/hex.c
- src/sim/sim_context.h
- src/sim/sim_context.c
- tests/test_ecs_entity.c
- tests/test_event_queue.c
- tests/test_hex.c
- tests/test_sim_tick.c
- tests/test_main.c
- docs/02_ARCHITECTURE_MAP.md
- docs/06_ECS_AND_EVENTS.md
- docs/07_NAVIGATION.md
- docs/12_TESTING.md
- backlog/active.md
- backlog/done.md
- backlog/todo.md
- backlog/sprints.md

## Decisions

- Keep queue and registry deterministic with explicit free-list ordering and explicit result codes.
- Keep `SimContext` minimal and headless to match sprint boundary while reserving command/event hooks for later tickets.
- Use deterministic fixed-size payload envelope with documented payload byte limit for event queue.
- Do not run build/test commands during this pass per session constraint.

## Tests

- tests added for entity registry, event queue, hex helpers, and sim tick behavior.
- full build/test execution not run (no explicit request).

## Next tasks

- CMD-001-command-queue-and-validation-contract
- WORLD-003-tile-field-storage
- EVT-003-serializable-event-trace-log
- NOISE-002-noise-event-and-field-impulse
- HORDE-002-attention-pressure-from-noise-field
