# TICKET ID: UI-002-command-staging-and-inspector

## Objective

Add a read-only command staging and inspection surface.

## Context

Player-facing controls must stage intent through commands and show validation results without directly mutating simulation state.

## Sprint

Sprint 9 - player-facing debug and control surfaces.

## Dependencies

- CMD-001-command-queue-and-validation-contract
- UI-001-sdl-debug-event-log-overlay

## Acceptance criteria

- UI staging state can build a command envelope without submitting it.
- Submission path uses the command queue/validation API.
- Validation errors are inspectable as result codes or short messages.
- Headless tests cover staging, valid submit, invalid submit, and no direct simulation mutation.
- SDL integration remains optional.

## Allowed files

- `src/ui/command_inspector.h`
- `src/ui/command_inspector.c`
- `tests/test_command_inspector.c`
- `tests/test_main.c`
- `src/platform/sdl_app.c`
- `CMakeLists.txt`
- `docs/08_UI_UX.md`

## Out of scope

- Full menu framework.
- Mouse selection rectangles.
- Keybind customization.
- Network commands.

## Required checks

- Format changed C files.
- Build with tests enabled.
- Run the command inspector test target or full `sim_tests` if tests remain single-binary.

## Notes

The inspector can explain why a command is invalid. It must not patch state to make the command valid.
