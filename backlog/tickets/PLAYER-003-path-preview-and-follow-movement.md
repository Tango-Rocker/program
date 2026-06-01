# TICKET ID: PLAYER-003-path-preview-and-follow-movement

# Title
Path preview and path-follow party movement

# Status
Done

# Context
Right-click movement previously issued an immediate relocation. RTS movement needs route preview, blocked-target rejection, and deterministic step-by-step movement through existing navigation systems.

# Acceptance Criteria

- The default scene builds path costs from the generated world map.
- Path preview uses the existing path service/pathfind APIs.
- Move orders resolve a route and store it as the active party path.
- The party advances along the route one deterministic scene update at a time.
- Movement appends `PARTY_MOVED` events per path step instead of teleporting.
- Headless tests cover preview, move order staging, path-follow completion, and movement event emission.

# Notes

This pass keeps movement immediate per update tick and does not yet animate interpolation between tiles.
