# TICKET ID: UI-007-command-alerts-and-feedback

# Title
Command history, alerts, and target feedback

# Status
Done

# Context
RTS controls need visible order/result feedback. The HUD now exposes recent commands, alerts, staged action state, hover targeting state, path summaries, and invalid command feedback.

# Acceptance Criteria

- Scene command surfaces append recent command summaries.
- Significant command consequences append alerts.
- The HUD renders command history and alerts in the inspector.
- The action bar shows active staged action and last command result.
- Invalid movement/interact/attack/noise outcomes produce visible summaries or alerts.

# Notes

This is a compact HUD feed, not a full scrollback log or notification system.
