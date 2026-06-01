# 16_TUTORIAL

The tutorial is a first-class player-facing system, not throwaway help text.

## Directive

- Every gameplay or UI change that alters how the player acts, reads feedback, or understands consequences must update the tutorial plan in this document or the implemented tutorial content.
- Tutorial content must explain player intent, controls, world feedback, and consequences without flattening the simulation into perfect prediction.
- Keep tutorial facts consistent with the current UI, command flow, event logs, field overlays, horde attention, jobs, topology, threat state, and causal reports.
- Tutorial work must stay non-authoritative: it can read state, surface prompts, and stage commands through existing command paths, but it must not mutate simulation state directly.

## Tutorial Plan

1. First contact: identify the party, camera movement, selection, pause, speed, and focus controls.
2. Movement: explain left-click selection, Move staging, right-click movement, path previews, and blocked or unreachable outcomes.
3. World response: show that noise, movement, combat, and interaction can feed fields, events, attention, logs, and later consequences.
4. Interaction: teach contextual `Interact` results for party, colony, worker, horde/contact, noise traces, and terrain.
5. Threats: explain horde anchors, materialized hostiles, attack targeting, command feedback, and alerts.
6. Investigation: introduce the inspector, event strip, field overlay, minimap filter, and causal panel as tools for understanding what happened.
7. Colony context: introduce jobs, worker state, stockpiles, construction, emergency state, and topology when those systems become player-visible.

## Current Implementation

- The SDL main menu includes a `Tutorial` screen with first-pass controls and consequence guidance.
- The tutorial screen is intentionally concise until dedicated tutorial state exists; it must still stay synchronized with implemented controls.
- The SDL main menu includes `Load` and `Settings` entries. `Load` currently reports that no save slots are available because persistent SDL save-slot UI is not implemented.
- The gameplay HUD now presents tutorial-relevant gameplay facts directly: party location, horde pressure, selected target, terrain, noise, route/order state, latest alert, and large action buttons.

## Maintenance Checklist

- Add or revise tutorial text/prompts when a control, command, or panel changes.
- Add or revise tutorial steps when a simulation system becomes visible to the player.
- Keep tutorial tests headless where possible by testing tutorial state and step eligibility separately from SDL rendering.
- Archive session notes with any tutorial decision, deferred step, or known mismatch.
