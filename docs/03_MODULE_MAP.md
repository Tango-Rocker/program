# 03_MODULE_MAP

## `src/core`

Core utilities used across all layers: logging, assertions, arena, RNG, and time.

## `src/platform`

SDL/app bootstrap, platform loop wrappers, and rendering/input boundary.

## `src/ecs`

Will own entity/component storage and systems. Placeholder directory for future ECS implementation.

## `src/event`

Will own event types, event queue, dispatch, and logging hooks.

## `src/world`

Contains map/chunk/hex world models, entities-to-world mapping, and topology data.
- Hex storage uses deterministic chunk keys and local offsets derived from cubic conversion.
- Chunk-local tile storage is now explicit and stored in deterministic chunk order for world-scale systems.
- Chunk lookup uses deterministic sorted search so large scenario generation does not linearly scan chunk storage.
- Topology artifacts expose stable integer keys intended for serialization and replay.
- Topology rebuilds use indexed passable-tile lookup for deterministic large-map region checks.
- Structure data is maintained by `src/world/structure.[ch]`:
  - Definition registry with footprint tile offsets.
  - Deterministic footprint placement and all-or-nothing occupancy mutation.
  - Blocking and portal flags encoded into world map tile values.
  - Topology dirty tracking to trigger downstream path/reachability rebuilds.

## `src/nav`

Navigation service, path requests, path handles, and spatial queries.
- A* path queries use caller-owned heap/touched scratch buffers, deterministic tie-breaking, and expanded-node budgets.

## `src/sim`

Deterministic simulation tick logic and system orchestration.

## `src/game`

Top-level application orchestration and bootstrap entry point.

## `src/colony`

Colony/workers/jobs/logistics/autonomous behaviors.
- `src/colony/construction.[ch]` provides deterministic construction worksite lifecycle with required
  resource accounting, haul-work coupling, and map placement handoff.
- `src/colony/emergency.[ch]` handles threat-driven interruption with resumable/abortable routine job policy
  and emergency audit trails.

## `src/lua`

Lua host integration and runtime script loading path.

## `src/script`

High-level scripted sequences and scenario glue.

## `src/render`

Render backends, cameras, overlays, and debug visualizations.

## `src/audio`

Sound scheduling, event-to-audio mapping, and future mixer services.

## `src/ui`

UI framework, status panels, logs, and command surfaces.
