# 01_AGENT_DIRECTIVES

These are the working rules for all agents touching this repository.

## Ticket workflow

1. Read `backlog/active.md`.
2. Open the ticket referenced there.
3. Read `docs/00_INDEX.md` and related modules.
4. Edit only allowed files in the ticket unless the ticket is unsafe.
5. Implement minimal behavior for the ticket.
6. Add/adjust tests where behavior is testable headlessly.
7. Update `docs/16_TUTORIAL.md` or implemented tutorial content when gameplay or UI changes affect how players learn controls, feedback, or consequences.
8. Update ticket state in backlog files when meaningful.
9. Add a session archive note for this work.

## Required local checks

- Ensure deterministic code paths are deterministic.
- Add tests for new behavior.
- Maintain headless testability where possible.
- Keep compile errors localized to the ticket scope.

## Editing conventions

- Prefer small, reviewable changes.
- Favor explicit error/result codes over implicit behavior.
- Use C17 plain structs and explicit ownership.
- Do not mutate simulation from UI directly.
- Keep Lua boundaries clear and one-directional where possible.
- Log key transitions and failure reasons.

## Safety defaults

- No global mutable state unless documented.
- No broad refactors during a ticket.
- No ad-hoc runtime systems without tests.
