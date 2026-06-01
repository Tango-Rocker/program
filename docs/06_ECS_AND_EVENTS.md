# 06_ECS_AND_EVENTS

## Principles

- Entities are handles with generation counters.
- Components are plain data.
- Systems own behavior.
- Commands are requests.
- Events are facts.
- Events should not become hidden control-flow soup.

## Distinction

- Commands: intent/request.
- Direct system scheduling: deterministic core simulation.
- Events: cross-system consequences, UI/audio/debug reactions, delayed effects, causal traces.
- Persistent scheduled events: save/load relevant future consequences.

## Target direction

- Command layer builds intent from UI and AI decisions.
- Simulation systems validate and execute command effects deterministically.
- Event bus publishes facts for overlay, audio, and AI consumers.
- Replay/logging records event chains for debugging.

## ECS identifier model

- `EntityId` is represented as a stable pair:
  - `index` (32-bit stable slot index).
  - `generation` (32-bit wraparound-safe generation counter).
- ID creation policy:
  - `index` selects slot in entity tables.
  - `generation` is stored in a per-slot table and increments on free/reuse.
  - Handles are invalid when `slot.generation != queried generation`.
- Deterministic guarantees:
  - Generation increments only via table-managed operations.
  - Slot reuse follows monotonic free-list order.
  - Debug checks reject stale writes to old generations.
- Sprint-1 foundation now includes a deterministic fixed-capacity registry with:
  - explicit `(index, generation)` handles
  - explicit invalid/empty result codes
  - deterministic FIFO free-list reuse by release order
  - stale-handle rejection after generation bump

## Event queue baseline

- Event envelope fields:
  - type
  - tick timestamp
  - source entity id
  - fixed-size payload (maximum documented size)
- Queue semantics:
  - fixed-capacity circular buffer
  - strict FIFO ordering for same-tick (and all) events
  - explicit result codes for push/pop/peek/clear
  - deterministic overflow behavior: closed, no mutation/corruption

## Command queue contract

- Command envelope fields mirror intent semantics:
  - command type
  - requested tick
  - source entity id
  - bounded payload bytes
- Validation rules are deterministic and explicit:
  - unsupported command type rejection
  - malformed source rejection
  - payload oversize rejection
  - stale/out-of-window tick rejection
  - fixed-capacity rejection

## Event trace log

- In-memory trace log stores causal sequence numbers and parent ids.
- Trace append preserves deterministic event order and supports stable text serialization for tests and session audits.

## Replay shell

- `src/sim/replay.c` applies ordered command fixtures through a deterministic command queue and emits a serialized event trace.
- Replay inputs capture:
  - RNG seed
  - total ticks to advance
  - ordered command list
- Replay comparison reports the first differing line in trace output.

## ECS storage draft

- Slot array owns generation and active masks.
- Component stores are dense vectors keyed by entity `index` with optional tombstone arrays.
- Component lookup policy:
  - direct index fetch for deterministic access on hot systems,
  - compacted dense storage where required for cache-friendly iteration.
- Deletion policy:
  - swap-delete inside dense component arrays,
  - entity index keeps `slot -> comp_index` indirection.

## Event model

- Typed event families:
  - command events: queued inputs/intent.
  - simulation events: deterministic facts from systems (state transitions).
  - audio/visual events: projection of simulation facts.
  - scheduled events: delayed execution with explicit due tick.
- Ownership:
  - simulation owns mutation of authoritative state.
  - command handlers own validation and translation to simulation mutations.
  - event bus carries immutable facts after mutation.
- Persistence rules:
  - event streams must be serializable by tick and ordered by source system.
  - same input + same initial state produces identical event ordering.
