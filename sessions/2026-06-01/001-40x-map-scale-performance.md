# Session 001 - 40x Map Scale Performance

## Goal

Increase the default map from 180x120 to 1140x760 while keeping pathfinding, topology, and minimap rendering bounded enough for headless tests and interactive UI.

## Files touched

- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/PERF-002-40x-map-scale-path-render.md`
- `docs/03_MODULE_MAP.md`
- `docs/07_NAVIGATION.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `src/game/default_scene.[ch]`
- `src/nav/path_service.[ch]`
- `src/nav/pathfind.[ch]`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.[ch]`
- `src/world/scenario_gen.c`
- `src/world/topology.c`
- `src/world/world_map.c`
- `tests/test_construction.c`
- `tests/test_default_scene.c`
- `tests/test_haul_job.c`
- `tests/test_path_service.c`
- `tests/test_pathfind.c`
- `tests/test_topology.c`
- `tests/test_ui_state.c`

## Decisions

- Targeted `1140x760` tiles, preserving the existing 3:2 shape and reaching about 40x the current tile count.
- Kept map storage deterministic but changed chunk lookup and insert-position discovery to binary search over sorted chunks.
- Sorted topology passable tiles and tile-region output so flood fill and region lookup use deterministic binary search.
- Reworked A* scratch to include caller-owned heap, heap positions, touched flags, and touched indexes.
- Changed path service budget consumption from request count to expanded-node count.
- Added default-scene topology prechecks and bounded route-corridor cost maps while reusing the full path scratch arena.
- Switched minimap terrain sampling to viewport-pixel-derived strides.

## Tests

- Passed `cmake --build build --target sim_tests`.
- Passed `build\Debug\sim_tests.exe`.

## Next tasks

- Build `sim_app` in an environment with SDL3 and SDL3_ttf installed.
- Add production path cache or chunk-level route cache if repeated long-distance AI traffic becomes common.
