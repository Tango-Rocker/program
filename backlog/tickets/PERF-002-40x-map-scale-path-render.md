# TICKET ID: PERF-002-40x-map-scale-path-render

# Title
40x map scale pathing and rendering performance

# Status
Done

# Context
The default interactive map needed to grow from `180x120` to roughly `40x` the current tile count without making path previews, movement, topology checks, or minimap rendering scale with full-map scans.

# Acceptance Criteria

- The default scene generates a `1140x760` map, about `866,400` tiles.
- World map chunk lookup remains deterministic and uses sorted lookup instead of linear chunk scans.
- Topology rebuild and tile-region lookup use deterministic indexed lookups.
- Pathfinding uses caller-owned heap/touched scratch and avoids full-grid open-list scans.
- Path service budget accounting is based on expanded nodes.
- Default-scene path requests precheck topology and run exact A* inside a bounded query corridor.
- Main world rendering remains visible-bounds scoped, and minimap terrain sampling is pixel-bounded.
- Headless tests cover the larger default map, path scratch behavior, topology indexed lookup, and minimap sampling caps.

# Allowed Files

- `src/world/**`
- `src/nav/**`
- `src/game/default_scene.[ch]`
- `src/ui/**`
- `tests/**`
- docs/backlog/session files

# Notes

This pass keeps simulation state deterministic and single-threaded. It does not add streaming terrain, persistent path caches, or production art.
