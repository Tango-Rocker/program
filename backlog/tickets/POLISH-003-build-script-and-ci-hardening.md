# TICKET ID: POLISH-003-build-script-and-ci-hardening

## Objective

Harden local build/test scripts and optional CI guidance.

## Context

The project now has many tests and optional SDL/Lua paths. Scripts should make common validation paths reliable without requiring manual command memory.

## Sprint

Sprint 12 - release-readiness polish and enhancement pass.

## Acceptance criteria

- Scripts support configure, build, test, and headless verification paths consistently.
- PowerShell and shell scripts document equivalent behavior or explicit platform limitations.
- Optional SDL/Lua absence is handled cleanly.
- Failure output points to the failed phase.
- README/testing docs list recommended quick and full validation commands.

## Allowed files

- `scripts/*.sh`
- `scripts/*.ps1`
- `README.md`
- `docs/12_TESTING.md`
- `CMakeLists.txt`

## Out of scope

- Hosted CI provider setup unless already present.
- Package manager integration.
- Container builds.
- New test behavior unrelated to script hardening.

## Required checks

- Run updated script paths that apply to the current environment.
- Build with tests enabled.
- Run tests.

## Notes

Do not make SDL mandatory. Headless verification remains the baseline.
