# 04_CODING_STANDARD

## C style

- Use C17 and keep files UTF-8.
- Use explicit structs and function-pointer callbacks over global implicit state.
- Prefer small functions and obvious control flow.
- Use one logical naming prefix per module (e.g., `game_log_*`, `game_rng_*`).

## Ownership and allocation

- Allocate through explicit functions; no hidden constructors.
- Arena allocation is linear and transient.
- Free in inverse allocation order is not required for arena memory.

## Header rules

- Public headers describe API and ownership expectations.
- Headers should not include implementation details.
- Keep `extern "C"` wrappers in C headers.

## Error handling

- Return explicit status codes.
- Log failures with context.
- Keep assertions for invariants that represent programmer errors.

## Logging

- Use `GAME_LOG_*` macros for runtime diagnostics.
- Keep fatal logs explicit and rare.

## Testing expectation

- All deterministic utilities should have unit tests.
- Seeded systems must be verifiable.
