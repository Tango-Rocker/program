# TICKET ID: BUFF-001-status-effect-lifecycle

## Objective

Implement deterministic status effect apply/tick/expire behavior.

## Context

Buffs and debuffs are scheduled simulation consequences. Their lifecycle needs explicit ownership and traceable expiration.

## Sprint

Sprint 6 - party actions and combat causal chain.

## Dependencies

- SCHED-001-deterministic-scheduled-events
- COMBAT-001-ability-command-resolution

## Acceptance criteria

- Status effect instance stores target entity, effect type, magnitude, applied tick, duration, and source id.
- Applying an effect validates target liveness and capacity.
- Expiration occurs by scheduled tick or deterministic update pass.
- Apply and expire actions emit trace/event facts.
- Headless tests cover apply, duplicate/stacking policy, expiration, stale target rejection, and trace order.

## Allowed files

- `src/sim/status_effect.h`
- `src/sim/status_effect.c`
- `tests/test_status_effect.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/11_PROJECTILES_BUFFS_PARTICLES.md`
- `docs/06_ECS_AND_EVENTS.md`

## Out of scope

- Full stat system.
- Complex aura rules.
- UI buff icons.
- Content-driven effect definitions.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the status effect test target or full `sim_tests` if tests remain single-binary.

## Notes

Document the first stacking policy clearly. Prefer one simple policy over implicit behavior.
