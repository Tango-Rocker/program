# TICKET ID: PLAYER-005-tactical-attack-command

# Title
Basic tactical attack command

# Status
Done

# Context
The demo already exercised combat internally, but the player-facing UI lacked an attack command. This adds a staged `Attack` action against horde/contact targets.

# Acceptance Criteria

- The action bar exposes an `Attack` button and staged attack mode.
- Attack target validation rejects non-hostile tiles.
- Valid horde/contact attacks submit through the scene command boundary.
- The command resolves through the existing combat ability path and appends combat events.
- Headless tests cover successful attack command resolution and command history/alert updates.

# Notes

This does not yet add projectile animation, ability cooldown UI, or multiple ability slots.
