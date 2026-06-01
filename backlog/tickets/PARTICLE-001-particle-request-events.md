# TICKET ID: PARTICLE-001-particle-request-events

## Objective

Add non-authoritative particle request events derived from simulation facts.

## Context

Particles are presentation requests. They should be traceable projections of combat/noise facts and must not own authoritative simulation state.

## Sprint

Sprint 6 - party actions and combat causal chain.

## Dependencies

- COMBAT-002-projectile-lifecycle-and-impact-events
- EVT-003-serializable-event-trace-log

## Acceptance criteria

- Particle request payload includes effect id, origin, tick, source event id, and bounded parameters.
- Requests are emitted from projectile impact or status events through a small projection layer.
- Request ordering is deterministic.
- Headless tests cover projection from impact, bounded payload rejection, and stable trace/log output.

## Allowed files

- `src/render/particle_request.h`
- `src/render/particle_request.c`
- `tests/test_particle_request.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/11_PROJECTILES_BUFFS_PARTICLES.md`
- `docs/08_UI_UX.md`

## Out of scope

- Actual particle rendering.
- GPU buffers.
- Particle simulation.
- Asset loading.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the particle request test target or full `sim_tests` if tests remain single-binary.

## Notes

The render layer may consume particle requests, but these requests must not feed back into authoritative simulation state.
