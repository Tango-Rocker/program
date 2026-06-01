# 10_HORDE_AI

## Horde LOD levels

- Level 0: dormant or abstract offscreen pressure
- Level 1: field-following horde agent
- Level 2: local avoidance and attack behavior
- Level 3: elite tactical behavior
- Level 4: boss behavior

Offscreen hordes may exist as abstract pressure groups and materialize near relevant spaces.
- Materialization requires threshold pressure, visibility/proximity context, and deterministic budgets; dematerialization returns pressure on actor removal.

## Current assumption

Milestone 0 defines no AI behaviors. Only a future event hooks placeholder strategy will be scaffolded.
- Current threat posture now feeds worker safety behavior through deterministic emergency scheduling:
  - Threat rise above worker thresholds may cause `COLONY` emergency interruption of in-progress routine jobs.
  - Emergency work is represented as higher-priority emergency board orders with auditability.
  - On threat clear, emergency system deterministically resumes or aborts stalled routine work according to policy.

## Noise-interest model

- Each horde actor/group maintains:
  - current pressure
  - last noise source id and age
  - attention threshold
  - current threat posture.
- Current minimal implementation updates pressure from a sampled noise field value, applies deterministic decay, and emits trace-backed posture transitions.
- Deterministic update:
  - if a `NoiseEmitted` field reaches a worker-defined threshold for a sector, pressure rises.
  - pressure decay is deterministic by elapsed ticks and terrain class.
  - posture transitions follow fixed hysteresis windows.
- LOD transitions:
  - abstract pressure group (`LOD0`) when no nearby player-visible actor.
  - projected flow-following (`LOD1`) when noise persists across multiple ticks.
  - active local behavior (`LOD2`) when path/collision visibility conditions are met.
  - tactical escalation (`LOD3`) if attacker lock-on and close distance thresholds hold.
