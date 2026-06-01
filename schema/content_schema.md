# Content Schema

This schema is intentionally small and fail-closed. Authored content is parsed into
C-owned immutable prototype sets before any simulation system can consume it.

Supported fixture rows:

- `ability "<id>" <damage> <cooldown_ticks>`
- `job "<id>" <priority> <work_ticks>`
- `effect "<id>" <duration_ticks> <magnitude>`

Rules:

- IDs must be non-empty, at most 31 bytes, and unique within each prototype type.
- Numeric values must be positive and within the C validator ranges.
- A valid content file must define at least one ability, one job, and one effect.
