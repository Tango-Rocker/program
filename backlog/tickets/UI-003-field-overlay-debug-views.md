# TICKET ID: UI-003-field-overlay-debug-views

## Objective

Add debug overlay data views for simulation fields.

## Context

Field overlays are key forensic tools. This ticket builds data extraction and optional display hooks without changing field values.

## Sprint

Sprint 9 - player-facing debug and control surfaces.

## Dependencies

- FIELD-001-multi-field-registry
- RENDER-002-hex-world-debug-renderer

## Acceptance criteria

- Overlay view can request a field id and bounded world area.
- Extracted overlay cells include coordinate, value, normalized display bucket, and source field id.
- Extraction does not mutate field storage.
- Headless tests cover field view extraction, missing field id, bounds clipping, and stable ordering.
- SDL display remains optional.

## Allowed files

- `src/ui/field_overlay.h`
- `src/ui/field_overlay.c`
- `tests/test_field_overlay.c`
- `tests/test_main.c`
- `src/render/hex_debug_render.c`
- `CMakeLists.txt`
- `docs/08_UI_UX.md`

## Out of scope

- Artist-facing color grading.
- Production minimap.
- Interactive editing of fields.
- Performance tuning for massive maps.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the field overlay test target or full `sim_tests` if tests remain single-binary.

## Notes

Overlay values are explanatory. They must not become gameplay inputs.
