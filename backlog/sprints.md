# Sprint Plan

## Current progress assessment

Progress level: Milestone 0 is complete. The project has now completed all twelve planned sprints.
Sprint 1 is complete with deterministic entity registry, fixed-capacity event queue, axial/cube helpers, and headless simulation ticking.
Sprint 2 is complete with deterministic command validation, bounded tile fields, event tracing, noise impulse application, and horde pressure transitions.
Sprint 3 is complete with deterministic pathing path-service behavior, stable job board reservations, and deterministic worker selection behavior.
Sprint 4 is now complete with replay trace capture, generated module index output, Lua content boundary stubs, and SDL forensic overlay wiring.
Sprints 5 through 10 are now complete, including topology indexing, scheduling persistence, combat/job/civic chains, sensory systems, logistics/construction, debug surfaces, and scenario/content tooling.
Sprints 11 and 12 are now complete, including architecture hardening, determinism stress coverage, docs/data hardening, build/test hardening, and known-issue burn-down.

Implemented foundation:

- CMake C17 project with optional SDL3 app target.
- `sim_core` library with logging, assertions, arena, RNG, and time primitives.
- Fixed-timestep app/platform skeleton.
- Headless tests for arena and RNG.
- Agent workflow, architecture docs, and completed seed backlog.

Currently implemented and documented:

- ECS entity/component scaffolding and deterministic storage APIs.
- Event queue, command queue, and event trace logging.
- Hex world storage and tile fields.
- Navigation pathfinding service and deterministic worker job selection.
- Replay harness, module inventory tooling, and Lua content boundary stubs.
- Debug overlay foundation for read-only forensic inspection.

Known risk level:

- The current architecture is internally consistent and deterministic for implemented subsystems.
- SDL remains optional and should not block headless tests.
- Remaining work is now focused on polish, API consolidation, and architecture hardening for future feature expansion.

## Progress levels

- Level 0: Foundation bootstrap. Complete.
- Level 1: Deterministic simulation kernel. Complete (Sprint 1 complete).
- Level 2: First causal world-hears-you chain. Complete.
- Level 3: Autonomous work and pathing. Complete.
- Level 4: Replayable, indexed, data-driven, inspectable runtime surface. Complete.
- Level 5: Scaled world storage, scheduling, and snapshots. Complete.
- Level 6: Party action and combat causal chain. Complete.
- Level 7: Sensory ecology and horde escalation. Complete.
- Level 8: Colony logistics and construction autonomy. Complete.
- Level 9: Player-facing debug/control surfaces. Complete.
- Level 10: Data, scenarios, and golden replay coverage. Complete.
- Level 11: Polish and architecture rework. Complete.
- Level 12: Release-readiness polish and enhancements. Complete.

## Sprint 1 - deterministic simulation kernel

Goal: make the smallest executable core that can create entities, queue facts, reason about hex coordinates, and advance a deterministic tick.

Tickets:

- ECS-002-entity-registry-implementation
- EVT-002-fixed-capacity-event-queue
- WORLD-002-hex-coordinate-module
- SIM-001-headless-sim-context-and-tick

Exit criteria:

- Entity handles reject stale generations.
- Event queue has deterministic FIFO ordering and explicit overflow behavior.
- Hex coordinate math is implemented and headlessly tested.
- A headless `SimContext` can advance ticks deterministically without SDL.

## Sprint 2 - first causal world-hears-you chain

Goal: prove the doctrine by implementing a narrow command-to-event-to-field-to-attention trace.

Tickets:

- CMD-001-command-queue-and-validation-contract
- WORLD-003-tile-field-storage
- EVT-003-serializable-event-trace-log
- NOISE-002-noise-event-and-field-impulse
- HORDE-002-attention-pressure-from-noise-field

Exit criteria:

- Commands are requests and do not mutate arbitrary state directly.
- Noise emits typed facts and applies deterministic field impulses.
- Horde pressure can rise/decay from field samples with hysteresis.
- Event trace output can explain the causal chain in stable order.

## Sprint 3 - autonomous work and pathing

Goal: add the first autonomous systems that can make choices from shared state without UI mutation.

Tickets:

- NAV-003-path-request-service-budgeting
- COLONY-002-job-board-storage-and-reservations
- COLONY-003-worker-job-selection-headless-sim

Exit criteria:

- Path queries are deterministic and stable under tie-breaks.
- Path handles expose explicit lifecycle states.
- Job reservations prevent duplicate claims and expire deterministically.
- Worker job selection emits an inspectable reason/audit fact.

## Sprint 4 - replay, indexes, data boundary, and debug surface

Goal: make the simulation easier for agents and developers to inspect, replay, and extend without weakening boundaries.

Tickets:

- REPLAY-001-deterministic-input-log-and-replay-shell
- TOOL-003-generated-module-index-prototype
- LUA-001-lua-data-boundary-stub
- UI-001-sdl-debug-event-log-overlay

Exit criteria:

- A simple replay fixture can reproduce event output from recorded seed and commands.
- Generated indexes are produced by tools, not edited manually.
- Lua/content boundary exists without hot ECS mutation.
- SDL debug overlay reads simulation/event state without mutating it.

## Sprint 5 - world scale, scheduling, and persistence

Goal: move from bounded subsystem fixtures toward a world state that can be scheduled, chunked, snapshotted, and replayed without losing determinism.

Tickets:

- WORLD-004-chunked-world-map-storage
- WORLD-005-region-topology-index
- SCHED-001-deterministic-scheduled-events
- SAVE-001-core-snapshot-format

Exit criteria:

- World tiles can be stored and queried through chunk keys and local offsets.
- Region/topology indexes can answer basic connected-area questions.
- Scheduled events persist as explicit tick-owned state, not hidden callbacks.
- Core snapshot output can round-trip enough state for deterministic tests.

## Sprint 6 - party actions and combat causal chain

Goal: implement the first party-driven combat chain from command to ability resolution to projectile, impact, status effect, particles, noise, and trace output.

Tickets:

- PARTY-001-party-actor-state-and-selection
- COMBAT-001-ability-command-resolution
- COMBAT-002-projectile-lifecycle-and-impact-events
- BUFF-001-status-effect-lifecycle
- PARTICLE-001-particle-request-events

Exit criteria:

- Party actors are authoritative simulation actors, not UI-only state.
- Ability commands validate through the command boundary.
- Projectile impact emits deterministic combat/noise/particle facts.
- Status effects apply and expire by tick.
- Visual/audio requests remain projections of simulation facts.

## Sprint 7 - sensory ecology and horde escalation

Goal: deepen the "world hears you" systems beyond noise by adding field families, horde LOD pressure groups, materialization rules, and non-authoritative audio requests.

Tickets:

- FIELD-001-multi-field-registry
- FIELD-002-light-scent-blood-field-updates
- HORDE-003-abstract-pressure-group-lod
- HORDE-004-materialization-and-dematerialization-rules
- AUDIO-001-event-to-audio-request-bus

Exit criteria:

- Multiple field families can update deterministically without duplicating storage logic.
- Hordes can exist as abstract pressure before local actors are spawned.
- Materialization/dematerialization has explicit deterministic rules.
- Audio requests are emitted from facts and never mutate simulation.

## Sprint 8 - colony logistics and construction

Goal: turn colony jobs into resource-moving and construction behavior that competes with threat interruptions.

Tickets:

- ECON-001-stockpile-and-inventory-storage
- COLONY-004-haul-job-chain
- COLONY-005-construction-worksite-lifecycle
- COLONY-006-emergency-threat-interruptions
- WORLD-006-structure-footprints-and-blocking

Exit criteria:

- Stockpiles and inventories are deterministic simulation state.
- Hauling jobs reserve source, destination, and resource quantity.
- Construction produces world changes through jobs and events.
- Threat interruptions leave explicit audit trails.
- Structures affect world blocking/topology through documented APIs.

## Sprint 9 - player-facing debug and control surfaces

Goal: make the implemented simulation slice inspectable and controllable without violating the command/system/event boundary.

Tickets:

- RENDER-001-camera-transform-and-party-leash
- RENDER-002-hex-world-debug-renderer
- UI-002-command-staging-and-inspector
- UI-003-field-overlay-debug-views
- UI-004-after-action-causal-report

Exit criteria:

- Camera math is deterministic and separate from world coordinates.
- Hex debug rendering can show the current simulated map.
- UI stages commands rather than mutating simulation state.
- Field overlays read snapshots only.
- After-action reports explain event chains from trace data.

## Sprint 10 - data, scenarios, and golden replay coverage

Goal: make authored content and generated scenarios testable through golden replay fixtures.

Tickets:

- DATA-001-content-schema-and-validator
- LUA-002-content-driven-ability-and-job-prototypes
- PROC-001-seeded-scenario-map-generator
- REPLAY-002-golden-scenario-replay-fixtures
- TOOL-004-sprint-status-report-generator

Exit criteria:

- Content validation fails closed with useful errors.
- Lua/content prototypes feed existing C-owned systems through bounded data.
- Seeded scenarios generate stable initial states.
- Golden replay fixtures catch behavioral drift.
- Sprint status can be generated from backlog files.

## Sprint 11 - polish and architecture rework pass

Goal: reduce accumulated friction after ten build-out sprints without changing product direction.

Tickets:

- REWORK-001-public-api-header-audit
- PERF-001-hot-loop-allocation-audit
- TEST-001-determinism-fuzz-smoke-suite
- DOC-002-architecture-drift-review
- UX-001-debug-overlay-legibility-pass

Exit criteria:

- Public headers expose small, documented APIs.
- Hot loops have explicit allocation behavior.
- Determinism smoke tests cover representative seeds and command streams.
- Docs match implementation reality.
- Debug overlays are legible enough to support future work.

## Sprint 12 - release-readiness polish and enhancement pass

Goal: finish a coherent demo-quality engineering slice and burn down known sharp edges.

Tickets:

- POLISH-001-error-handling-and-result-code-unification
- POLISH-002-log-message-and-causal-reason-cleanup
- POLISH-003-build-script-and-ci-hardening
- POLISH-004-known-issues-burn-down
- POLISH-005-demo-slice-readiness-pass

Exit criteria:

- Result codes and error logs are consistent across implemented systems.
- Causal reason strings/enums are useful for debugging and reports.
- Build/test scripts are reliable on supported local paths.
- Known issues are updated with fixed/deferred status.
- A small deterministic demo slice can be built, tested, and explained.
