# Session: Bootstrap foundation

## Goal

Bootstrap a durable foundation repository for the Autonomous Simulation Horde RTS/RPG project and complete Milestone 0.

## Scope

- project and build setup
- core utilities
- deterministic headless tests
- optional SDL app skeleton
- agent workflow docs and backlog
- session archive entry and scripts

## Files touched

- CMakeLists.txt
- AGENTS.md
- README.md
- .clang-format
- .clang-tidy
- .gitignore
- src/core/*
- src/platform/*
- src/game/main.c
- tests/*
- docs/00_INDEX.md through docs/15_AGENT_WORKFLOW.md
- backlog/* and backlog/tickets/*
- scripts/configure_debug.sh
- scripts/build_debug.sh
- scripts/test.sh
- scripts/format.sh
- sessions/2026-05-28/001-bootstrap-foundation.md

## Decisions

- Kept SDL optional to preserve headless build viability.
- Kept fixed timestep deterministic in core and SDL loop for bootstrap.
- Added seeded RNG and arena with tests as baseline deterministic utilities.
- Added ticket-driven workflow docs before substantial feature coding.

## Known issues

- Existing SDL3 sources are not implemented beyond runtime bootstrap.
- Existing `src/main.c` at repository root is not part of CMake targets.
- Full ECS/event bus/nav/colony/horde systems remain future tickets.

## Tests run

- `cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug -DGAME_ENABLE_SDL=ON -DGAME_ENABLE_TESTS=ON`
- `cmake --build build-debug`
- `ctest --test-dir build-debug --output-on-failure`

## Next tasks

- validate fixed-timestep behavior for both headless and SDL targets with smoke runtime test
- expand deterministic test coverage for arena overflow and alignment edge cases
- create docs for build reproducibility and session index generation
