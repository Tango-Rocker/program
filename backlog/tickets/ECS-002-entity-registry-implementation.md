# TICKET ID: ECS-002-entity-registry-implementation

## Objective

Implement the baseline ECS entity registry with generation-counted handles.

## Context

`ECS-001` documented the identifier model. This ticket turns that contract into a small, tested C module without adding components or systems yet.

## Sprint

Sprint 1 - deterministic simulation kernel.

## Acceptance criteria

- `EntityId` stores `index` and `generation` as explicit fixed-width fields.
- Registry create/destroy/is-alive operations are deterministic and return explicit result codes or invalid handles.
- Destroying an entity invalidates stale handles after generation increment.
- Reused slots follow deterministic free-list ordering.
- Headless tests cover create, destroy, stale handle rejection, and slot reuse.

## Allowed files

- `src/ecs/entity.h`
- `src/ecs/entity.c`
- `tests/test_ecs_entity.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/06_ECS_AND_EVENTS.md`
- `docs/12_TESTING.md`

## Out of scope

- Component storage.
- System scheduling.
- Serialization.
- Event emission on entity lifecycle changes.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the ECS entity test target or full `sim_tests` if tests remain single-binary.

## Notes

Keep the public API small. Do not introduce hidden global registry state.
