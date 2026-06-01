# TICKET ID: HORDE-002-attention-pressure-from-noise-field

## Objective

Implement a minimal horde attention pressure update from noise field samples.

## Context

`HORDE-001` documented pressure, attention thresholds, and posture transitions. This ticket adds the smallest deterministic consumer of the noise field.

## Sprint

Sprint 2 - first causal world-hears-you chain.

## Dependencies

- NOISE-002-noise-event-and-field-impulse
- EVT-003-serializable-event-trace-log

## Acceptance criteria

- Horde attention state stores pressure, last source id, last source age, threshold, and posture.
- Update consumes field samples and deterministic elapsed ticks.
- Pressure rise and decay follow documented integer or fixed-point rules.
- Posture transitions use hysteresis so threshold jitter does not flap state every tick.
- Threat/posture changes emit or trace an `ActorAttentionUpdated` or `ThreatStateChanged` fact.
- Headless tests cover rise, decay, no-flap hysteresis, and event/trace emission.

## Allowed files

- `src/sim/horde_attention.h`
- `src/sim/horde_attention.c`
- `tests/test_horde_attention.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/10_HORDE_AI.md`
- `docs/14_SIMULATION_DOCTRINE.md`
- `docs/12_TESTING.md`

## Out of scope

- Local movement or attacks.
- Pathfinding integration.
- LOD materialization.
- Rendering indicators.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the horde attention test target or full `sim_tests` if tests remain single-binary.

## Notes

This ticket should prove causality, not full AI. Keep state small and inspectable.
