# TICKET ID: PLAYER-004-contextual-world-interactions

# Title
Contextual world interaction outcomes

# Status
Done

# Context
`Interact` previously appended a generic event. It now needs to surface different outcomes for important world targets so the UI feels like it is interacting with the simulation, not a placeholder button.

# Acceptance Criteria

- Interacting with colony, worker, horde/contact, noise, party, and open terrain produces distinct scene interaction kinds/summaries.
- Interactions still submit through the scene command boundary.
- Invalid/blocked targets are rejected without direct UI mutation.
- Interaction events continue to append to the event log.
- Headless tests cover contextual colony and worker interactions.

# Notes

This pass produces contextual summaries and event facts. It does not yet open dedicated management panels or inventory screens.
