# 07_NAVIGATION

## Navigation plan

- hex grid authoritative
- region/portal graph
- path request budget
- path handles
- flow fields for hordes
- spatial index
- local steering
- navmesh later only if proven useful

## Milestone 0 scope

The navmesh remains unimplemented initially. The hex-grid model is the canonical source of movement and accessibility.

## Axial/cube conversion

- Axial coordinates use `(q, r)` with offset basis:
  - cube conversion: `(x, y, z) = (q, r, -q-r)`
  - inverse conversion: `(q, r) = (x, z)`
- Neighbor offsets (axial): `(+1, 0), (0, +1), (-1, +1), (-1, 0), (0, -1), (+1, -1)`
- Hex distance:
  - `dist(a, b) = max(|x1-x2|, |y1-y2|, |z1-z2|)` using cube coords.

## Hex indexing and chunk strategy

- Chunked storage splits world into fixed axial ranges:
  - chunk key `(cx, cy, cz)` derived from floor-dividing cube coordinates by `CHUNK_RADIUS`.
  - local tile offset stored as remapped non-negative index in chunk cell.
- Deterministic index:
  - stable chunk key ordering by lexicographic tuple.
  - stable local index by linearized `(x, y, z)` offsets.
- Serialization rule:
  - store chunk key + local tile offset to reconstruct absolute tile deterministically.
- Sprint-1 implementation now provides helpers for:
  - axial<->cube conversion
  - six-neighbor lookup in deterministic order
  - cube distance
  - chunk key/local offset derivation with explicit floor-division behavior for negative coordinates
- Sprint-5 implementation adds chunk storage by explicit chunk allocation, deterministic lookup/create behavior, and stable iteration over stored chunks/tiles.

## Future services

- Global path cache for repeated requests.
- Region-based cost fields for group movement.
- Horde flow field generation keyed by threat/noise.

## Synchronous path query

- Added deterministic bounded A* query in `src/nav/pathfind.c`.
- Query inputs are bounded maps with explicit cost data (`UINT16_MAX` as blocked), explicit node budget, and caller-owned scratch buffers.
- Neighbor expansion order follows the deterministic axial neighbor ordering from `world/hex`.
- Tie-break is deterministic and stable when `f` and `g` scores collide.
- Navigation services treat structures as terrain modifiers via world map flags from `src/world/structure.c`.
  - Footprints write deterministic blocking bits for passability checks.
  - Structure updates set map-level topology-dirty markers for rebuild hooks.
  - Blocked footprints participate in pathing and topology region partitioning.

## Path request API and service implementation

  - Request handle structure:
    - `PathRequest { request_id, start, goal, constraints, requested_at_tick }`
    - `PathHandle { id, state, version, expires_at_tick, result_version }`
  - States:
  - `PENDING`: request accepted, computation deferred.
  - `RESOLVED`: result available, immutable path payload for this version.
  - `FAILED`: no route found for the supplied constraints.
  - `CANCELLED`: request explicitly revoked before resolution.
  - `EXPIRED`: stale handle with no consumer before TTL.
  - Lifecycle:
    - submit returns an immutable handle id and initial `PENDING`.
    - path service advances pending requests with deterministic node budgets.
    - once resolved, path consumers must include `result_version` to prove replay compatibility.
    - TTL is deterministic; expired handles are marked `EXPIRED` and then cleaned after the expiry tick.

## Budget and cancellation

- Budget guidance:
  - per-frame path node budget is clamped to a deterministic maximum.
  - highest priority requests are path-critical actors (combat, flee, immediate player-visible movement).
  - low-priority background requests can be deferred and coalesced by origin/destination.
- Cancellation:
  - UI or AI can cancel by handle before `RESOLVED`.
  - cancellation clears budget reservations immediately.
  - cancellation of in-progress work is still deterministic: mark handle `CANCELLED`, keep accounting entry for logs/replay.

## Layer boundaries

- Navigation API must not depend on render state or camera coordinates.
- AI behavior reads resolved paths only; it does not mutate path internals.
