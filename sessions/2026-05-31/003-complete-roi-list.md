# Session 003 - Complete ROI List

## Goal

Complete the rest of the ROI list after path preview, minimap navigation, and contextual interactions.

## Files touched

- `README.md`
- `backlog/active.md`
- `backlog/done.md`
- `backlog/tickets/PLAYER-005-tactical-attack-command.md`
- `backlog/tickets/UI-007-command-alerts-and-feedback.md`
- `backlog/tickets/UI-008-pause-speed-minimap-filter-inspector.md`
- `docs/08_UI_UX.md`
- `docs/12_TESTING.md`
- `src/game/default_scene.c`
- `src/game/default_scene.h`
- `src/game/main.c`
- `src/sim/command.c`
- `src/sim/command.h`
- `src/ui/sdl_hud.c`
- `src/ui/ui_state.c`
- `src/ui/ui_state.h`
- `tests/test_default_scene.c`
- `tests/test_ui_state.c`

## Decisions

- Added a staged `Attack` action and `ATTACK_TARGET` command type for horde/contact targets.
- Recorded compact command history and alerts in the default scene for HUD projection.
- Added pause/speed controls to UI state and made app update respect them.
- Added a minimap noise overlay toggle.
- Expanded inspector output with path status, command history, alerts, and attack summaries.
- Added compact hover tooltip labels for world targets and controls.

## Tests

- Added default-scene coverage for attack command resolution, command history, alerts, and attack summary.
- Added UI coverage for attack staging, pause toggle, speed cycling, and minimap filter toggling.
- Added UI hit-label smoke coverage for tooltips.
- Passed targeted `sim_tests` and `sim_app` builds before full verification.
- Passed `.\scripts\generate-indexes.ps1`.
- Passed `.\scripts\verify-debug.ps1`.
- Passed `cmake --build cmake-build-debug --target sim_app`.
- Passed `.\cmake-build-debug\sim_tests.exe`.
- Passed `ctest --test-dir cmake-build-debug --output-on-failure`.
- Passed `git diff --check`.
- Passed `sim_app` launch smoke; the app started and was closed after the smoke interval.
- Confirmed generated sprint status and backlog done entries include `PLAYER-003`, `PLAYER-004`, `PLAYER-005`, `UI-006`, `UI-007`, and `UI-008`.

## Next tasks

- Add animated interpolation for party movement and combat effects.
- Add real ability cooldown/range UI when combat grows beyond one demo attack.
- Replace compact feeds with full log/notification widgets if the HUD becomes crowded.
