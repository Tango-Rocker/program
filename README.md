# Autonomous Simulation Horde RTS/RPG

A top-down 2D hex-grid RTS/RPG simulation scaffold focused on deterministic, headless-testable core systems and an optional SDL3 runtime.

## Project summary

This repository starts with a narrow but strong foundation:

- CMake build setup
- Core primitives (logging, assertions, arena, RNG, timing)
- Fixed-timestep app skeleton
- Headless tests for core systems
- Optional SDL3 app target
- Agent-first docs and ticket workflow

## Build requirements

- C compiler with C17 support (Clang preferred)
- CMake >= 3.20
- Optional: SDL3 and SDL3_ttf dev packages for app target

## Configure debug build

```bash
./scripts/configure_debug.sh
```

The script creates `build-debug` and enables tests.

## Build

```bash
./scripts/build_debug.sh
```

The CMake build also provides a headless default app target:

```bash
cmake --build build-debug --target sim_headless
./build-debug/sim_headless
```

## Run tests

```bash
./scripts/test.sh
```

## Headless build if SDL3 is missing

If SDL3 is unavailable, use:

```bash
cmake -S . -B build-debug -DGAME_ENABLE_SDL=OFF -DGAME_ENABLE_TESTS=ON
cmake --build build-debug
cmake --build build-debug --target sim_tests
```

`sim_tests` remains buildable and runnable without SDL.

## Run SDL app on Windows/MSYS2 UCRT

After installing SDL3 and SDL3_ttf through MSYS2 UCRT, make sure the runtime DLL path is visible before launching:

```powershell
$env:PATH = "C:\Users\Tango\tools\msys64\ucrt64\bin;$env:PATH"
cmake --build cmake-build-debug --target sim_app
.\cmake-build-debug\sim_app.exe
```

The SDL app boots a deterministic 100x-area showcase map. The HUD supports tile selection, camera-centered map projection, minimap navigation and filters, path preview and path-follow party movement, contextual world interaction, tactical attack, command/alert feedback, pause/speed controls, field overlays, and the noise command chain.

## Repository structure

- `src/` core engine sources and platform bootstrap
- `include/` public headers and future API surface
- `tests/` headless C tests
- `docs/` architecture and workflow docs
- `backlog/` ticket tracking and status
- `scripts/` local build/format/test automation
- `data/`, `schema/`, `generated/`, `tools/` future pipeline folders

## Agent workflow summary

- Current task is in `backlog/active.md`.
- Follow `AGENTS.md` and `docs/00_INDEX.md`.
- Keep each change scoped to the ticket.
- Log results in `sessions/YYYY-MM-DD/`.

## Current milestone

Current engineering slice:

- deterministic default scene initialized by `src/game/main.c`
- larger seeded showcase map distributing party, colony, stockpile, and horde anchors across 21600 tiles
- SDL HUD with selectable hex map, status bar, party action panel, inspector, event strip, and collapsible forensic panels
- RTS-style camera/input layer with right-click movement, staged action targeting, minimap jumps, focus buttons, and path previews
- compact RTS HUD feedback with attack command, command history, alerts, pause/speed controls, minimap noise filter, and richer inspector diagnostics
- `Emit Noise` action feeding field and horde attention consequences through the command queue
- deterministic demo chain covering sensory fields, horde materialization, colony emergency/construction, combat/projectiles/status effects, audio/particle requests, and replay trace capture
- colony worker autonomy audit in the same scene setup
- replay, causal-report, content-schema, scenario-generator, and headless regression tests
- optional SDL app loop when SDL3 is available
