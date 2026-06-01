# TICKET ID: CORE-001-cmake-skeleton

## Objective

Create the build skeleton for CMake, optional SDL target, and test target configuration.

## Context

Milestone 0 requires a deterministic, headless-friendly build that can run even without SDL.

## Acceptance criteria

- `CMakeLists.txt` exists with required options.
- `GAME_ENABLE_SDL` and `GAME_ENABLE_TESTS` are present.
- Project can be configured with and without SDL.
- Compile commands are generated.

## Allowed files

- `CMakeLists.txt`
- `.clang-format`
- `.clang-tidy`

## Out of scope

- Gameplay implementation.
- Full dependency vendoring.

## Required checks

- `cmake -S . -B build-debug -DGAME_ENABLE_SDL=OFF -DGAME_ENABLE_TESTS=ON`
- `cmake --build build-debug`

## Notes

SDL target should be optional and skipped with a clear warning when not found.
