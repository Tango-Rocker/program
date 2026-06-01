# Lua Data Boundary

`data/lua` currently holds authored content fixtures consumed by the C-side boundary host.

The parser is intentionally small and deterministic:

- one `prototype` entry per line
- syntax: `prototype "<role_id>" <stamina> <threat_threshold>`
- entries are immutable once loaded into `GameLuaWorkerPrototypeSet`

This keeps parsing deterministic and keeps script execution separate from simulation mutation.
