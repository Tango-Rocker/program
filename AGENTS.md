# AGENTS.md

## Prime directive

Make the smallest correct change that satisfies the current ticket while preserving the large-scale simulation architecture.

This project is intentionally ambitious. Do not shrink the design just because it is large. Instead, contain risk with tickets, tests, deterministic simulation, generated indexes, and explicit module boundaries.

## Product doctrine

The desired feeling is:

> The spreadsheet under the floor coughed.

The player is an actor inside an autonomous world. The simulation may be partially opaque to the player, but it must be internally consistent, testable, and traceable.

The core fantasy is:

> The world hears you.

Actions should eventually feed systems such as events, fields, horde attention, jobs, topology, threat state, and logs.

## Before editing

- Read the current backlog ticket.
- Read `docs/00_INDEX.md`.
- Read relevant module docs.
- Search before adding new names or systems.
- Use LSP/diagnostics where available.
- Check `docs/13_KNOWN_ISSUES.md`.
- Prefer existing patterns over invention.

## During editing

- Stay inside the ticket’s allowed files unless the ticket is defective.
- Do not edit generated files directly.
- Do not introduce hidden global mutable state.
- Do not allocate in hot loops unless documented.
- Do not make UI mutate simulation state directly.
- Use commands for intent, systems for state changes, and events for cross-system consequences.
- Preserve deterministic behavior in core simulation code.
- Keep Lua at content/behavior boundaries, not arbitrary hot ECS mutation.
- Keep public APIs small and documented.

## After editing

- Format changed C files.
- Build.
- Run targeted tests.
- Add or update tests for new behavior.
- Update docs if architecture changed.
- Update backlog status.
- Add a session archive entry describing goal, files touched, decisions, tests, and next tasks.

## Generated files

Files in `/generated` are produced by tools. Do not edit them manually. Edit schemas, source definitions, or generator code.

## Error handling

Prefer explicit result codes in core systems. Log useful context. Avoid silent failure.

## C style

Use C17. Prefer plain structs and explicit ownership. Avoid clever macro machinery unless it reduces boilerplate safely and is documented.

## Testing

Any system that can be tested headlessly should be testable without SDL.

## Scope control

Do not implement unrelated systems while working a ticket. Big project, small cuts.
