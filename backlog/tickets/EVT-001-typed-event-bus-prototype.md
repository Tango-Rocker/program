# TICKET ID: EVT-001-typed-event-bus-prototype

## Objective

Prepare the event bus direction and typed event model.

## Context

Simulation-first architecture depends on deterministic event logging and delayed consequences.

## Acceptance criteria

- event taxonomy and ownership boundaries documented.

## Allowed files

- `docs/06_ECS_AND_EVENTS.md`
- `docs/12_TESTING.md`

## Out of scope

- production event bus code.

## Required checks

- clear command/event distinction.

## Notes

Commands are request; events are facts.
