# TICKET ID: TOOL-002-build-format-test-scripts

## Objective

Create bootstrap scripts for configure, build, test, and formatting.

## Context

Agents need deterministic local workflows with low setup cost.

## Acceptance criteria

- scripts exist and run with basic inputs.
- scripts remain Unix-like and minimal.

## Allowed files

- `scripts/configure_debug.sh`
- `scripts/build_debug.sh`
- `scripts/test.sh`
- `scripts/format.sh`

## Out of scope

- advanced CI automation.

## Required checks

- configure, build, and test scripts can be invoked from a fresh shell.

## Notes

Scripts should document optional SDL behavior when unavailable.
