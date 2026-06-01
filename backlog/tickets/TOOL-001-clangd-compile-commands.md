# TICKET ID: TOOL-001-clangd-compile-commands

## Objective

Make clangd-friendly workflows consistent with compile_commands generation.

## Context

Agents will need reliable symbol discovery and diagnostics.

## Acceptance criteria

- `compile_commands.json` generation enabled.
- CMake exposes header include paths for both src and include.

## Allowed files

- `CMakeLists.txt`

## Out of scope

- IDE-specific project files.

## Required checks

- Ensure `compile_commands.json` is produced after configure.

## Notes

Current milestone relies on default CMake output.
