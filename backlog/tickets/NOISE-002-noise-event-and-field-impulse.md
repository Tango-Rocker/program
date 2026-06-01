# TICKET ID: NOISE-002-noise-event-and-field-impulse

## Objective

Implement the first `NoiseEmitted` event payload and deterministic field impulse application.

## Context

This is the first concrete "the world hears you" slice. A command or system should be able to emit noise as a fact and apply its field consequence in a testable way.

## Sprint

Sprint 2 - first causal world-hears-you chain.

## Dependencies

- EVT-002-fixed-capacity-event-queue
- WORLD-002-hex-coordinate-module
- WORLD-003-tile-field-storage
- SIM-001-headless-sim-context-and-tick

## Acceptance criteria

- `NoiseEmitted` payload includes origin, intensity, max radius, decay/attenuation parameter, source id, and source tick.
- Applying a noise impulse updates tile field values deterministically by distance from origin.
- Out-of-bounds tiles are skipped with explicit accounting or result status.
- A `FieldImpulseApplied` event or trace entry records the causal parent noise event.
- Headless tests cover radius, attenuation, clamping, and causal trace emission.

## Allowed files

- `src/sim/noise_system.h`
- `src/sim/noise_system.c`
- `tests/test_noise_field.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/02_ARCHITECTURE_MAP.md`
- `docs/14_SIMULATION_DOCTRINE.md`
- `docs/12_TESTING.md`

## Out of scope

- Audio playback.
- UI noise meter.
- Horde attention changes.
- Multiple field backends.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the noise field test target or full `sim_tests` if tests remain single-binary.

## Notes

Use integer or otherwise deterministic arithmetic. Do not use platform-dependent floating behavior in core field propagation unless documented and tested.
