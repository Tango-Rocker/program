# TICKET ID: UI-004-after-action-causal-report

## Objective

Generate a compact after-action causal report from event trace data.

## Context

The player and developer should be able to inspect why something happened after the fact. This ticket converts trace chains into readable summaries.

## Sprint

Sprint 9 - player-facing debug and control surfaces.

## Dependencies

- EVT-003-serializable-event-trace-log
- COMBAT-002-projectile-lifecycle-and-impact-events
- HORDE-002-attention-pressure-from-noise-field

## Acceptance criteria

- Report generator can follow causal parent links from a selected event id.
- Output order is stable and bounded.
- Report includes tick, event type, source id, and reason code where available.
- Missing parent links are reported explicitly.
- Headless tests cover linear chain, branching source selection, missing parent, and stable text output.

## Allowed files

- `src/ui/causal_report.h`
- `src/ui/causal_report.c`
- `tests/test_causal_report.c`
- `tests/test_main.c`
- `CMakeLists.txt`
- `docs/08_UI_UX.md`
- `docs/14_SIMULATION_DOCTRINE.md`

## Out of scope

- Natural language generation.
- Full player journal.
- Persistent report files.
- UI layout.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the causal report test target or full `sim_tests` if tests remain single-binary.

## Notes

Prefer precise structured summaries over vague prose. The report is a debugging instrument first.
