# TICKET ID: COMBAT-001-ability-command-resolution

## Objective

Resolve a minimal ability command into deterministic combat intent facts.

## Context

Combat must use the command boundary. This ticket validates a simple ability request and emits facts that later projectile/status systems consume.

## Sprint

Sprint 6 - party actions and combat causal chain.

## Dependencies

- CMD-001-command-queue-and-validation-contract
- PARTY-001-party-actor-state-and-selection
- EVT-003-serializable-event-trace-log

## Acceptance criteria

- Ability command validates actor, target, range, cooldown, and ability id.
- Accepted ability command emits an `AbilityUsed` trace/event fact.
- Rejected ability command returns explicit reason and does not mutate combat state.
- Cooldown or use timing is deterministic by tick.
- Headless tests cover valid use, invalid actor, out-of-range target, cooldown rejection, and stable event trace output.

## Allowed files

- `src/sim/combat.h`
- `src/sim/combat.c`
- `tests/test_combat_ability.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/11_PROJECTILES_BUFFS_PARTICLES.md`
- `docs/12_TESTING.md`

## Out of scope

- Projectile movement.
- Damage formulas beyond minimal fixture values.
- UI action bar.
- Content-driven ability catalogs.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the combat ability test target or full `sim_tests` if tests remain single-binary.

## Notes

Keep formulas simple and deterministic. Content extensibility comes in a later sprint.
