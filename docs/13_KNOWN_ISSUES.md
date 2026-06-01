# 13_KNOWN_ISSUES

- SDL3 and SDL3_ttf availability may vary by environment.
- Event bus remains thin (fixed-capacity queue/trace only) and is not yet integrated through a full production dispatcher.
- Lua integration is currently a bounded parser/content-validation boundary, not an embedded Lua VM.
- Replay coverage is fixture-level and trace-based; it is not a full save/load compatibility harness.
- Generated indexes and sprint status reports exist, but must be regenerated after module/backlog changes.
- SDL HUD rendering is optional; headless tests are the authoritative validation path in environments without SDL3/SDL3_ttf.
