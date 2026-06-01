# TICKET ID: WORLD-005-region-topology-index

## Objective

Add a deterministic region/topology index for connected areas and portals.

## Context

Navigation, horde pressure, construction blocking, and colony jobs need fast answers about connected regions without each system inventing its own topology model.

## Sprint

Sprint 5 - world scale, scheduling, and persistence.

## Dependencies

- WORLD-004-chunked-world-map-storage
- NAV-002-hex-a-star-path-query

## Acceptance criteria

- Region ids are stable for the same input map and rebuild order.
- Topology rebuild consumes passability data from world storage.
- Portal/neighbor relationships are stored in deterministic sorted order.
- API can answer whether two tiles are in the same region.
- Headless tests cover disconnected regions, bridge/portal connection, blocking changes, and stable region ids.

## Allowed files

- `src/world/topology.h`
- `src/world/topology.c`
- `tests/test_topology.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/03_MODULE_MAP.md`
- `docs/07_NAVIGATION.md`
- `docs/12_TESTING.md`

## Out of scope

- Incremental topology updates.
- Render overlays.
- Multithreaded rebuilds.
- Full nav cache invalidation.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the topology test target or full `sim_tests` if tests remain single-binary.

## Notes

If full rebuild is simplest, use full rebuild. The first implementation should favor correctness and determinism over clever incremental behavior.
