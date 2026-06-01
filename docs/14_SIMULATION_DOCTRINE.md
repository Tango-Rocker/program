# 14_SIMULATION_DOCTRINE

## Core doctrine

- The player is not owed omniscience.
- The simulation is owed consistency.
- The developer is owed traceability.
- The agent is owed tests and indexes.

## Forensic legibility

The UI may not predict every consequence, but after an event chain occurs, the engine should be able to expose enough state, logs, fields, and causal breadcrumbs for debugging and eventual player-facing investigation tools.

## Engineering implication

Deterministic inputs, seeded randomness, and event logs are required before adding higher-level automation.

## Noise chain and forensic traceability

- Sensory stack:
  - light/scent/blood are separate fields using the shared registry and deterministic tile storage.
  - field decay, clamping, and impulse application are deterministic and bounded.

- Noise contract:
  - source handle, intensity, audible radius, start tick, and decay half-life.
  - source may be action-driven (attack, movement, environmental).
- Field processing:
  - noise contributes to a deterministic field impulse at source tile.
  - decay is applied per tick on each affected tile or aggregate bucket.
  - values are clamped and serialized deterministically for replay.
- Event chain:
  - `NoiseEmitted` -> `FieldImpulseApplied` -> `ActorAttentionUpdated` -> `ThreatStateChanged`.
  - all derived facts carry tick, source handle, causal parent sequence, and deterministic payload.
- Construction and emergency chains:
  - `WorksiteCreated` -> haul request -> haul transfer -> `ConstructionProgressUpdated` -> `StructurePlaced`.
  - `ThreatExceeded` -> `RoutineJobStalled` -> `EmergencyOrderCreated` -> `EmergencyOrderCompleted` or `ThreatCleared`.
  - emergency clear transitions are deterministic (`resume routine` or `abort routine`) and should be reflected in the emergency audit trail.
- Worker actions should also emit explicit reasons for autonomous decisions (ex: selected, stalled on threat, avoided reservation, no valid candidate) so post-hoc inspection can reconstruct why routine labor changed state.
- UI/forensics:
  - logs can reconstruct why actors reacted by following chained event ids in order.

## Replay and boundary doctrine

- Replay fixtures are deterministic inputs with explicit seed, tick horizon, ordered command stream, and trace comparison outputs.
- Replay mismatch reporting should identify first differing trace line for deterministic forensics.
- Lua/content boundaries return immutable values and explicit parse/validation statuses, and should not expose mutable simulation handles.
- Read-only overlays are acceptable to inspect state; they must not mutate simulation data.
- Scenario generation is deterministic fixture setup: explicit seed/config in, bounded world state and anchors out.
- Camera state and debug render surfaces are projections. They may follow or inspect simulation state but remain non-authoritative.
- The default app scene is a deterministic engineering slice: one party action produces traceable noise/field/horde consequences while colony autonomy records an audit entry.
