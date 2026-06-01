# 12_TESTING

## Milestone 0 test scope

- unit tests
- headless simulation tests
- deterministic seed tests
- event log tests (planned)
- pathfinding tests (later)
- replay tests (later)
- sanitizer builds
- coordinate conversion tests (later)
- entity registry and registry lifecycle tests
- fixed-capacity event queue tests
- headless simulation tick determinism tests

## Current tests

- arena allocation/reset/alignment/overflow
- RNG determinism and value range
- entity create/destroy/stale rejection/slot reuse behavior
- event queue ordering, clear/empty/pop semantics, and overflow handling
- hex conversion, neighbor order, distance, and chunk/local indexing
- deterministic simulation tick advancement (init, invalid config, repeatable RNG state)
- command queue validation and FIFO ordering
- bounded tile field storage semantics
- chunked world map storage creation/query/update/iteration semantics
- deterministic event trace append/parenting
- noise field impulse radius/attenuation
- horde attention rise/decay/hysteresis
- deterministic path queries and stable tie-breaking
- path service lifecycle tests (submit/status/cancel/retrieve, expiry, budgeting, stale handles)
- job board storage/reservation lifecycle tests (create, reserve conflict, expiry, valid/invalid transitions)
- worker job selection tests (role match, urgency/distance tie-break, stale reservation avoidance, threat preemption, audit reasons)
- construction worksite lifecycle tests (planning, resource delivery, build ticks, completion placement, abort handling, audit trail)
- structure footprint tests (placement validation, blocking footprint updates, removal rollback restoration, deterministic footprint ordering, topology dirty flag)
- emergency interruption tests (routine stall, reservation release, emergency priority, resume/abort paths, audit output)
- replay fixture tests (capture deterministic trace and mismatch line reporting)
- golden replay fixture tests for combat/noise, colony-haul, and horde-pressure slices
- Lua boundary load tests (missing fixture, valid parse, malformed fixture)
- content schema tests (valid prototypes, missing required rows, duplicate ids, out-of-range values)
- camera tests (round-trip transforms, zoom clamp, party leash non-mutation)
- command inspector tests (stage, validate, submit, invalid rejection without queue mutation)
- field overlay tests (bounded extraction, missing field, clipping/order, non-mutation)
- causal report tests (linear chain, branch selection, missing parent)
- scenario generator tests (seed stability, seed/config difference, invalid config, topology sanity)
- default scene test (100x area showcase map, startup slice includes party noise, horde reaction, worker audit, causal report, HUD-submitted noise command effects, party movement, and world interaction)
- default scene pathing/interactions test (path preview, path-follow movement completion, movement events, contextual colony and worker interactions)
- default scene tactical command feedback test (attack rejection before hostile materialization, attack command resolution after materialization, command history, alerts, attack summary, and event-queue preservation)
- default scene demo showcase test (sensory fields, horde materialization, colony emergency/construction, combat/projectile/status, audio/particle projection, replay trace)
- UI state/layout tests (camera-centered large-map projection, visible tile bounds, tile and marker hit-testing, marker priority, movement/interact/noise action staging, button hit-testing, panel collapse state, inspector-safe compact controls, and stable desktop/compact layouts)
- UI minimap tests (minimap coordinate mapping, minimap click hit testing, and focus-button camera/selection behavior)
- UI controls tests (pause toggle, speed cycle, minimap filter toggle, and attack action staging)
- UI tooltip label smoke coverage for hit targets.
- module index generator smoke checks (generated file refresh and stability)
- sprint status generator stability checks
- SDL HUD compile coverage (SDL3 + SDL3_ttf app target only)
- field registry tests (id lookup, lifecycle, disabled access, deterministic field ordering)
- sensory field tests (light/scent/blood impulse + decay + trace emission + sampling)
- abstract horde group tests (field tick updates, deterministic migration tie-breaks, posture events)
- horde materialization tests (spawn/dematerialize determinism and trace links)
- audio request queue tests (projection, deterministic ordering, overflow handling, payload validation)

## Test command guidance

- configure: `cmake -S . -B build -DGAME_ENABLE_TESTS=ON`
- build: `cmake --build build`
- run: `ctest --test-dir build`
- quick run executable: `./build/sim_tests`
- default scene executable: `./build/sim_headless`
- SDL app executable: `./build/sim_app`

Sanitizers are optional and can be enabled with `-DGAME_ENABLE_SANITIZERS=ON`.
