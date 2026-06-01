# Session: Fixed timestep loop hardening

## Goal

Implement a shared fixed-delta-time application loop flow for both headless and SDL builds with consistent init/update/render/shutdown sequencing.

## Scope

- Refactor app-loop sequencing into a single path in `game_app_run`.
- Add optional platform hooks for polling/render-pre/post in the app configuration.
- Update SDL app entrypoint to use the shared loop and keep optional-SDL behavior.
- Update ticket tracking for completed work and move next suggestions.

## Files touched

- src/platform/app.h
- src/platform/app.c
- src/platform/sdl_app.c
- backlog/active.md
- backlog/done.md
- sessions/2026-05-28/003-fixed-timestep-loop.md

## Decisions

- Kept ticket changes confined to platform loop layer and ticket artifacts.
- Added optional hooks rather than SDL-only behavior in the core loop to keep deterministic behavior shared.
- Added `platform_user_data` to preserve simulation/user data separation for hooks.
- Added explicit `max_ticks` validation to prevent accidental negative loops.
- Ensured shutdown callback is only invoked when initialize callback has run, matching established lifecycle semantics.

## Tests

- Not run in this pass (per instruction to avoid extra validation unless explicitly requested).

## Next tasks

- TOOL-001-clangd-compile-commands
- CORE-003-log-assert-arena-rng
- ECS-001-entity-id-generation-storage
