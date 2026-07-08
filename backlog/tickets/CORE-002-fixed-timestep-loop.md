# TICKET ID: CORE-002-fixed-timestep-loop

## Objective

Implement fixed-timestep simulation scaffolding with initialize, accumulate, step, render callback, and shutdown flow.

## Context

Bootstrap gameplay is not required. A deterministic loop is required for future simulation systems.

## Acceptance criteria

- fixed dt loop in app platform code.
- clear frame/time accumulation behavior.
- shutdown path in all branches.

## Allowed files

- `src/platform/app.h`
- `src/platform/app.c`
- `src/platform/sdl_app.c`
- `src/core/time.h`
- `src/core/time.c`
- `src/game/main.c`

## Out of scope

- Simulation gameplay systems.

## Required checks

- Build target for app and tests.
- headless loop exits cleanly.

## Notes

SDL path should use optional compile path and share timing behavior with headless.
