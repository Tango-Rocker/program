# 02_ARCHITECTURE_MAP

## Layer overview

- Platform Layer
- Core Layer
- Input Layer
- UI Layer
- Command Layer
- ECS Layer
- Event Bus
- World Layer
- Navigation Service
- Simulation Layer
- Lua/Data Layer
- Render Layer
- Audio Layer
- Agent Harness

## Planned flow

- UI emits intent.
- Command system validates intent.
- Simulation changes state.
- Events report results.
- UI reflects state.
- Headless authoritative simulation loop advances a `SimContext` with:
  - explicit tick counter
  - deterministic RNG state
  - entity registry storage
  - fixed-capacity event queue

## Responsibilities

- Platform Layer owns windowing, process init/shutdown, and optional graphics/audio backends.
- Core Layer owns fixed timestep, timing, RNG, logging, memory primitives, and cross-cutting utilities.
- Input Layer converts OS/device input into stable intent events.
- UI Layer renders instruments and allows command staging/inspection.
- Command Layer validates and queues high-level requested actions.
- ECS Layer stores entities/components and schedules deterministic simulation systems.
- Event Bus transports observable consequences and delayed effects.
- World Layer stores map, topology, fields, and actor clusters.
- Navigation Service computes routes and local steering.
- Simulation Layer runs deterministic update systems: jobs, entities, horde pressure, economy.
- Lua/Data Layer provides scripted content and behavior values.
- Render Layer draws world and overlays.
- Audio Layer surfaces deterministic feedback tied to event signals via request projection queues.
- Agent Harness executes tickets, runs tests, and records sessions.

Milestone 0 only includes core utilities, optional SDL entrypoint, and deterministic test scaffolding.

## Noise and horde signal chain (foundational)

- `NoiseEmitted` facts carry origin, intensity, max radius, decay curve, and tick timestamp.
- Deterministic field updates apply attenuation and clamping to produce per-tile influence totals.
- Horde systems consume the field snapshot each tick and raise threat-state transitions where thresholds are crossed.
- UI/audio/log consumers receive derived facts from the same deterministic source event.
- Audio projections are non-authoritative and cannot feed simulation state.
- Command processing path now includes deterministic queue validation and a logged causal chain (`NoiseEmitted` -> `FieldImpulseApplied` -> `ActorAttentionUpdated`), with bounded field storage as the shared world signal surface.
