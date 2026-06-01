# TICKET ID: UI-008-pause-speed-minimap-filter-inspector

# Title
Pause, speed, minimap filters, and richer inspector

# Status
Done

# Context
The remaining RTS usability gaps were lightweight simulation time controls, minimap filtering, and denser inspector feedback.

# Acceptance Criteria

- Top bar exposes pause and speed controls.
- Keyboard shortcuts can toggle pause and cycle speed.
- Scene updates respect pause and speed multiplier.
- Minimap can toggle a noise overlay filter.
- Inspector includes path status, recent commands, alerts, and attack summaries.
- Headless UI tests cover pause, speed, minimap filter, and attack action staging.

# Notes

This pass does not add a full settings menu, keybinding editor, or persistent UI preferences.
