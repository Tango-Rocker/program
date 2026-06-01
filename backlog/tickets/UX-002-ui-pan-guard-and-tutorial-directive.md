# TICKET ID: UX-002-ui-pan-guard-and-tutorial-directive

## Status

Done.

## Goal

Prevent UI hover/drag from panning the camera and establish a standing tutorial maintenance directive.

## Allowed files

- `src/game/main.c`
- `src/ui/ui_state.[ch]`
- `tests/test_ui_state.c`
- `docs/00_INDEX.md`
- `docs/01_AGENT_DIRECTIVES.md`
- `docs/08_UI_UX.md`
- `docs/16_TUTORIAL.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/UX-002-ui-pan-guard-and-tutorial-directive.md`
- `sessions/2026-05-31/005-ui-pan-guard-and-tutorial-directive.md`

## Acceptance criteria

- Hovering over top bar, bottom panel, right inspector, or event strip does not edge-pan the camera.
- Right-drag camera panning is ignored while the pointer is over UI chrome.
- The pan guard is covered by headless UI tests.
- Tutorial maintenance is documented as a directive and linked from the docs index.
