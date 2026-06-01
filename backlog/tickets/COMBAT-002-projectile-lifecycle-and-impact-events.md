# TICKET ID: COMBAT-002-projectile-lifecycle-and-impact-events

## Objective

Implement a minimal projectile lifecycle from spawn to impact event.

## Context

The combat chain needs a concrete moving consequence that emits impact, damage, noise, and visual request facts without relying on rendering.

## Sprint

Sprint 6 - party actions and combat causal chain.

## Dependencies

- COMBAT-001-ability-command-resolution
- NOISE-002-noise-event-and-field-impulse

## Acceptance criteria

- Projectile state stores source, position, target, speed/tick step, remaining lifetime, and payload id.
- Projectile update is deterministic and tick-based.
- Impact emits a `ProjectileImpact` fact and a causal trace parented to `AbilityUsed`.
- Impact can trigger minimal damage and noise facts through existing event/trace systems.
- Headless tests cover spawn, movement, lifetime expiration, impact, and causal chain ordering.

## Allowed files

- `src/sim/projectile.h`
- `src/sim/projectile.c`
- `tests/test_projectile.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/11_PROJECTILES_BUFFS_PARTICLES.md`
- `docs/14_SIMULATION_DOCTRINE.md`

## Out of scope

- Sprite interpolation.
- Complex collision volumes.
- Homing behavior.
- Area-of-effect damage.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the projectile test target or full `sim_tests` if tests remain single-binary.

## Notes

Use integer or fixed-step coordinate behavior. Rendering can interpolate later but must not alter authoritative projectile state.
