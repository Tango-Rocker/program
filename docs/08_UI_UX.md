# 08_UI_UX

The UI does not fully explain the world in advance. It provides instruments, warnings, stale reports, logs, overlays, and forensic tools. The player may be surprised, but the simulation must not be arbitrary.

## Planned UI elements

- top status bar
- bottom RPG panel
- party portraits
- action bar
- minimap
- logs
- field overlays
- noise meter
- sound pings
- camera leash

## UX principles

- Favor actionable clues over perfect prediction.
- Preserve uncertainty while making causes inspectable through forensics.
- Keep overlays lightweight and readable.
- Use logs to reveal causal breadcrumbs over time.

## Current player HUD milestone

- `src/ui/ui_state.[ch]` owns non-authoritative selection, hover, panel visibility, command result, and layout/hit-test state.
- `src/ui/sdl_hud.[ch]` renders the SDL3/SDL3_ttf player HUD for `sim_app` only: hex map, top status bar, party/action panel, event strip, right inspector, and collapsible forensic panels.
- The HUD hex geometry is scaled to fit the larger default showcase map at desktop and compact viewports while preserving marker-priority hit testing.
- The HUD uses a camera-centered world projection for the 100x area demo map, keeping nearby tiles clickable while the underlying map remains much larger than the viewport.
- RTS-style input polish includes right-click movement, staged action targeting from the bottom action bar, clamped keyboard/edge camera panning, hover targeting feedback, and bounded hit testing over the visible world area.
- The HUD includes a full-map minimap with party/colony/horde markers, a viewport rectangle, click-to-jump camera navigation, and focus buttons for the primary anchors.
- Move targeting previews the route and active move orders step the party along that route through deterministic scene updates.
- `Interact` now reports context-specific outcomes for party, colony, worker, horde/contact, noise traces, and open terrain.
- The action bar includes a staged tactical `Attack` command that resolves against horde/contact targets through the command/combat path.
- The HUD surfaces recent command history, alerts, attack summaries, active action state, last command result, and richer selected-tile diagnostics in the inspector.
- Top-bar pause/speed controls and keyboard shortcuts gate scene updates, while the minimap can toggle a noise overlay filter.
- Hovered controls and world targets expose compact tooltip labels near the playfield.
- UI chrome blocks hover, hit-test fallthrough, keyboard-menu, edge, and drag camera panning; only the active world playfield can trigger camera pan gestures.
- The SDL app opens fullscreen on a main menu with `New Game`, `Load`, `Settings`, and `Tutorial` screens. `Load` is currently a placeholder because persistent save slots are not implemented.
- The gameplay HUD prioritizes readable player-facing state over debug telemetry: party position, horde posture/pressure, current target, terrain/noise, current order, latest alert, route summary, and large action buttons.
- Debug-style seed/tick/event counts and demo-chain summaries are not part of the main gameplay HUD.
- Tutorial guidance is tracked in `docs/16_TUTORIAL.md` and mirrored in the SDL tutorial menu; both must be updated as player-facing controls, panels, and consequence explanations change.
- Production-hardening pass: attack targeting now requires a materialized hostile actor, command entry points preserve pending events, compact action controls stay outside the side panel, visible tile bounds drive map rendering/hit testing, path previews are debounced by origin/target, and text labels are cached in the SDL HUD.
- `src/platform/sdl_app.c` forwards SDL platform events to the app layer so UI input can stage commands without mutating simulation state directly.
- `src/game/default_scene.[ch]` exposes small command-processing surfaces used by the HUD to submit `Move`, `Interact`, and `Emit Noise` intent. Movement and interaction update authoritative scene state through the scene command boundary; `Emit Noise` continues through the command inspector, command queue, noise field, horde attention, event log, and causal report chain.
- `src/game/default_scene.[ch]` also owns the deterministic demo showcase status: sensory field impulses, horde group/materialization, colony emergency/construction, combat/projectile/status, audio/particle request projection, and replay trace capture are summarized in the inspector.
- Fonts are loaded from `C:\Windows\Fonts\segoeui.ttf` first, then `arial.ttf`; the HUD remains shape-only with a warning if no font is available.
- `src/render/camera.[ch]` keeps camera transform and party leash math outside authoritative simulation state.
- `src/render/hex_debug_render.[ch]` collects non-authoritative hex debug cells and display colors from read-only map data.
- `src/ui/command_inspector.[ch]` stages commands and submits them through the command queue instead of mutating simulation state.
- `src/ui/field_overlay.[ch]` extracts bounded field views from registry snapshots without changing field storage.
- `src/ui/causal_report.[ch]` writes compact parent-chain reports from event trace data.
