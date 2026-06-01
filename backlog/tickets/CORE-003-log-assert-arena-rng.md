# TICKET ID: CORE-003-log-assert-arena-rng

## Objective

Add foundational core modules for logging, assertions, arena allocator, and deterministic RNG.

## Context

These primitives are required by nearly all future systems and should be minimal but stable.

## Acceptance criteria

- logger with TRACE/DEBUG/INFO/WARN/ERROR/FATAL.
- assert macro exists.
- arena supports init/push/reset/destroy and alignment checks.
- RNG supports seed + u32 + float01 deterministic output.
- tests cover arena and RNG.

## Allowed files

- `src/core/log.h`
- `src/core/log.c`
- `src/core/asserts.h`
- `src/core/arena.h`
- `src/core/arena.c`
- `src/core/random.h`
- `src/core/random.c`
- `tests/test_arena.c`
- `tests/test_random.c`

## Out of scope

- Lua binding.
- full event bus.

## Required checks

- `ctest` against `sim_tests`.
- deterministic RNG sequence for a fixed seed.

## Notes

Keep APIs small and test-first.
