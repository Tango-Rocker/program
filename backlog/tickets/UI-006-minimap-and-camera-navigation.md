# TICKET ID: UI-006-minimap-and-camera-navigation

# Title
Minimap and strategic camera navigation

# Status
Done

# Context
The 180x120 map is too large to navigate with only direct camera panning. The HUD needs a strategic minimap and focus controls for key anchors.

# Acceptance Criteria

- The SDL HUD renders a minimap for the full generated map.
- Minimap markers show party, colony, and horde anchors.
- A viewport rectangle indicates current camera focus.
- Clicking the minimap jumps the camera to the corresponding tile.
- Focus buttons jump to/select party, colony, and horde anchors.
- Headless UI tests cover minimap tile mapping, minimap hit testing, and focus selection.

# Notes

This pass does not add fog of war, minimap filters, or drag-panning from the minimap.
