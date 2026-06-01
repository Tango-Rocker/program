# Session: Complete WORLD-004 chunked world map storage

## Goal
- Complete `WORLD-004-chunked-world-map-storage` (Sprint 5 ticket).

## Files touched
- `src/world/world_map.h`
- `src/world/world_map.c`
- `tests/test_world_map.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/03_MODULE_MAP.md`
- `docs/07_NAVIGATION.md`
- `docs/12_TESTING.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/todo.md`
- `backlog/sprints.md`

## Implementation decisions
- Added explicit chunked world map API with deterministic chunk creation, chunk-keyed lookup, explicit result codes, and stable iteration over chunk/tile order.
- Stored chunk tile data in sorted deterministic chunk order by lexicographic chunk key.
- Prevented implicit chunk allocation in `get`/`set` by returning `GAME_WORLD_MAP_RESULT_CHUNK_MISSING`.
- Added headless tests covering:
  - init/create/get/set behavior
  - missing chunk failure
  - positive and negative coordinate handling
  - explicit chunk/local access
  - stable deterministic chunk/tile iteration ordering

## Tests
- Added `test_world_map` and wired it into `tests/test_main.c`.

## Next tasks
- Continue Sprint 5 with `WORLD-005-region-topology-index`.
