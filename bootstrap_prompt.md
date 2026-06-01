# Bootstrap Prompt: Autonomous Simulation Horde RTS/RPG

Use this prompt as the first message to a coding agent, such as Codex, when initializing the repository from an empty or near-empty folder.

---

You are initializing a new long-horizon game project from an empty or near-empty repository.

This is not a toy prototype. This is a deliberately ambitious simulation-heavy top-down 2D hex-grid horde survival RTS/RPG built in C with SDL3, Lua-driven content, ECS, event bus, navigation services, colony simulation, horde AI, deterministic headless simulation, and an agent-first software engineering workflow.

The project’s desired feeling is:

> The spreadsheet under the floor coughed.

The player is not the center of the universe. The player-controlled RPG party is one autonomous actor-cluster inside a vast world with its own rules: jobs, monsters, fields, hunger, light, scent, blood, heat, rituals, logistics, pathing, panic, error states, and consequences. The world should sometimes feel larger than the player’s understanding.

The core fantasy remains:

> The world hears you.

Player and colony actions create consequences through sound, light, scent, danger, topology changes, combat, blood, construction, magic resonance, work orders, and horde attention.

The engineering goal is to build a repository that can survive large scope under sustained agentic development. Favor explicit tickets, tests, deterministic simulation, generated indexes, session archives, diagnostics, coding standards, and module boundaries. Do not shrink the design because it is large. Make it survivable.

Your task for this first run is to initialize the repository structure, build system, agent instructions, documentation, backlog, scripts, and Milestone 0 executable/test skeleton.

Do not implement the whole game. Build the launchpad, not the moon.

---

## Product vision

Create a top-down 2D hex-grid game in C using SDL3 where the player primarily controls an RPG party, while a surrounding autonomous simulation continues to operate: colony/base workers, jobs, stockpiles, monsters, fields, dungeons, rituals, noise propagation, light, scent, horde pressure, and tactical combat.

The party is the player’s body, but not the whole world. The camera is party-bound with a leash, but the world continues beyond direct command and direct observation.

Core loop:

```txt
Party explores, fights, loots, builds, opens, breaks, sneaks, chants, burns, bleeds
  -> actions emit commands and events
  -> events alter fields, jobs, topology, faction state, monster interest, worker behavior
  -> world systems react autonomously
  -> consequences propagate through noise, scent, light, nav, job queues, threat state, and horde attention
  -> player survives, investigates, adapts, extracts, upgrades, and dives deeper
```

---

## Design doctrine

### 1. Simulation-first, not player-flattering

The simulation is allowed to surprise the player. It should not always be cleanly forecast. However, it must be internally consistent, inspectable through debug tools, and eventually explainable through logs, field views, investigation, scout reports, aftermath reports, or forensic UI.

Bad opacity:

```txt
The player loses and cannot infer why because systems are arbitrary or bugs are hidden.
```

Good opacity:

```txt
The player loses and later realizes the forge, broken door, blood trail, worker panic, and open gate formed a causal chain.
```

The UI should not spoil every consequence, but the engine must preserve causal traces.

### 2. The world hears you

Noise, scent, light, danger, heat, blood, magic resonance, construction, combat, and movement are simulation inputs.

Noise is not only a UI meter. Treat it as:

```txt
an event stream
an impulse into a field
an AI attractor
a forensic trace
a source of systemic escalation
a possible tactical resource
```

### 3. Party-first, world-not-party-only

The player mostly acts through an RPG party. Colony and RTS systems support the expedition fantasy, but they are real systems, not decorations. Workers, monsters, jobs, structures, fields, and abstract horde groups can act without direct player attention.

### 4. Agent-first engineering

The repository must be easy for coding agents to work in safely:

```txt
small tickets
clear allowed files per ticket
generated indexes
session archives
module maps
tests
deterministic headless simulation
build scripts
clangd support
clear AGENTS.md instructions
no hidden architectural traps
```

### 5. Determinism and observability are first-class

Every important systemic consequence should be reproducible or traceable.

Plan early for:

```txt
fixed timestep
seeded RNG
headless simulation
event logs
ECS stats dumps
replay hooks
debug overlays later
testable systems
causal event chains
```

---

## Technical baseline

Use:

```txt
Language: C17
Build: CMake
Compiler preference: Clang
Editor/LSP: clangd with compile_commands.json
Platform: SDL3 for app/window/input/rendering, but keep headless tests independent where possible
Scripting/content: Lua, planned but not required in Milestone 0
Testing: simple C test runner for now
Formatting: clang-format config
Static analysis: clang-tidy config if available
Sanitizers: debug sanitizer target or documented command
```

Do not vendor large dependencies in this first run.

If SDL3 is unavailable in the current environment, the repository must still configure and build headless tests. The SDL app target may be optional behind a CMake option. Document this clearly.

---

## Required repository layout

Create this structure. Empty directories may use `.gitkeep` where necessary.

```txt
/
  AGENTS.md
  CMakeLists.txt
  README.md
  .clang-format
  .clang-tidy
  .gitignore

  /src
    /core
    /platform
    /ecs
    /event
    /world
    /nav
    /sim
    /game
    /colony
    /lua
    /script
    /render
    /audio
    /ui

  /include
    /game

  /data
    /lua

  /schema

  /generated

  /docs
    00_INDEX.md
    01_AGENT_DIRECTIVES.md
    02_ARCHITECTURE_MAP.md
    03_MODULE_MAP.md
    04_CODING_STANDARD.md
    05_DATA_AND_LUA.md
    06_ECS_AND_EVENTS.md
    07_NAVIGATION.md
    08_UI_UX.md
    09_COLONY_JOBS.md
    10_HORDE_AI.md
    11_PROJECTILES_BUFFS_PARTICLES.md
    12_TESTING.md
    13_KNOWN_ISSUES.md
    14_SIMULATION_DOCTRINE.md
    15_AGENT_WORKFLOW.md

  /backlog
    active.md
    todo.md
    done.md
    /tickets

  /sessions

  /tests

  /tools
    /generators
    /validators
    /indexers
    /agent_tools

  /scripts
```

---

## Required first-run implementation

Implement Milestone 0 only.

Milestone 0 includes:

```txt
CMake project
C17 configuration
optional SDL3 app target
headless core/test target
fixed timestep skeleton
logging
assert macro
basic memory arena
seeded RNG
basic time utilities
compile_commands.json support
clangd-friendly setup
basic test runner
initial docs
initial backlog tickets
session archive entry for this bootstrap run
```

### Minimum C modules to create

Create the following real files with compiling stubs or minimal implementation:

```txt
src/core/log.h
src/core/log.c
src/core/asserts.h
src/core/arena.h
src/core/arena.c
src/core/random.h
src/core/random.c
src/core/time.h
src/core/time.c

src/platform/app.h
src/platform/app.c
src/platform/sdl_app.c

src/game/main.c

tests/test_main.c
tests/test_arena.c
tests/test_random.c
```

The app does not need gameplay. It may open a window if SDL3 is available, run a fixed timestep loop, log ticks, and exit cleanly. If SDL3 is unavailable, the headless tests must still build.

### Required scripts

Create simple scripts:

```txt
scripts/configure_debug.sh
scripts/build_debug.sh
scripts/test.sh
scripts/format.sh
```

They should be portable for Unix-like environments, simple, and documented in README. Do not assume exotic local tooling.

---

## CMake requirements

CMake should provide:

```txt
option(GAME_ENABLE_SDL "Build SDL app target" ON)
option(GAME_ENABLE_TESTS "Build tests" ON)
```

If SDL3 is not found and `GAME_ENABLE_SDL=ON`, either:

1. fail with a clear message that says how to configure headless mode, or
2. automatically skip only the SDL target and keep tests available.

Prefer clarity over cleverness.

Generate `compile_commands.json`.

Use useful warnings for C development. Do not make warnings-as-errors mandatory yet.

---

## AGENTS.md requirements

Create a root `AGENTS.md` with this content or a faithful equivalent:

```md
# AGENTS.md

## Prime directive

Make the smallest correct change that satisfies the current ticket while preserving the large-scale simulation architecture.

This project is intentionally ambitious. Do not shrink the design just because it is large. Instead, contain risk with tickets, tests, deterministic simulation, generated indexes, and explicit module boundaries.

## Product doctrine

The desired feeling is:

> The spreadsheet under the floor coughed.

The player is an actor inside an autonomous world. The simulation may be partially opaque to the player, but it must be internally consistent, testable, and traceable.

The core fantasy is:

> The world hears you.

Actions should eventually feed systems such as events, fields, horde attention, jobs, topology, threat state, and logs.

## Before editing

- Read the current backlog ticket.
- Read `docs/00_INDEX.md`.
- Read relevant module docs.
- Search before adding new names or systems.
- Use LSP/diagnostics where available.
- Check `docs/13_KNOWN_ISSUES.md`.
- Prefer existing patterns over invention.

## During editing

- Stay inside the ticket’s allowed files unless the ticket is defective.
- Do not edit generated files directly.
- Do not introduce hidden global mutable state.
- Do not allocate in hot loops unless documented.
- Do not make UI mutate simulation state directly.
- Use commands for intent, systems for state changes, and events for cross-system consequences.
- Preserve deterministic behavior in core simulation code.
- Keep Lua at content/behavior boundaries, not arbitrary hot ECS mutation.
- Keep public APIs small and documented.

## After editing

- Format changed C files.
- Build.
- Run targeted tests.
- Add or update tests for new behavior.
- Update docs if architecture changed.
- Update backlog status.
- Add a session archive entry describing goal, files touched, decisions, tests, and next tasks.

## Generated files

Files in `/generated` are produced by tools. Do not edit them manually. Edit schemas, source definitions, or generator code.

## Error handling

Prefer explicit result codes in core systems. Log useful context. Avoid silent failure.

## C style

Use C17. Prefer plain structs and explicit ownership. Avoid clever macro machinery unless it reduces boilerplate safely and is documented.

## Testing

Any system that can be tested headlessly should be testable without SDL.

## Scope control

Do not implement unrelated systems while working a ticket. Big project, small cuts.
```

---

## Documentation requirements

Create initial docs with real content, not placeholders.

### docs/00_INDEX.md

Describe what each doc is for and how agents should navigate the docs.

### docs/01_AGENT_DIRECTIVES.md

Expand the rules from `AGENTS.md` and explain the ticket workflow.

### docs/02_ARCHITECTURE_MAP.md

Include the high-level architecture:

```txt
Platform Layer
Core Layer
Input Layer
UI Layer
Command Layer
ECS Layer
Event Bus
World Layer
Navigation Service
Simulation Layer
Lua/Data Layer
Render Layer
Audio Layer
Agent Harness
```

Also include:

```txt
UI emits intent.
Command system validates intent.
Simulation changes state.
Events report results.
UI reflects state.
```

### docs/03_MODULE_MAP.md

List the intended responsibility of each `/src` subdirectory.

### docs/04_CODING_STANDARD.md

Define C style, naming, ownership, allocation, header rules, error handling, logging, testing, and formatting expectations.

### docs/05_DATA_AND_LUA.md

Describe the planned Lua boundary:

```txt
Lua defines data and high-level behavior.
C owns hot loops, memory, ECS, navigation, rendering, audio, deterministic simulation.
Lua does not directly mutate arbitrary ECS memory.
```

### docs/06_ECS_AND_EVENTS.md

Define the intended ECS/event direction, including:

```txt
Entities are handles with generation counters.
Components are plain data.
Systems own behavior.
Commands are requests.
Events are facts.
Events should not become hidden control-flow soup.
```

Include this distinction:

```txt
Commands: intent/request.
Direct system scheduling: deterministic core simulation.
Events: cross-system consequences, UI/audio/debug reactions, delayed effects, causal traces.
Persistent scheduled events: save/load relevant future consequences.
```

### docs/07_NAVIGATION.md

Describe the planned layered navigation system:

```txt
hex grid authoritative
region/portal graph
path request budget
path handles
flow fields for hordes
spatial index
local steering
navmesh later only if proven useful
```

The original long-form plan includes navmesh, but the first implementation should not start there. The hex grid is authoritative.

### docs/08_UI_UX.md

Define the UI philosophy:

```txt
The UI does not fully explain the world in advance. It provides instruments, warnings, stale reports, logs, overlays, and forensic tools. The player may be surprised, but the simulation must not be arbitrary.
```

Include planned elements:

```txt
top status bar
bottom RPG panel
party portraits
action bar
minimap
logs
field overlays
noise meter
sound pings
camera leash
```

### docs/09_COLONY_JOBS.md

Describe work orders, jobs, reservations, workers, roles, threat states, and autonomous behavior.

### docs/10_HORDE_AI.md

Describe horde LOD:

```txt
Level 0: dormant or abstract offscreen pressure
Level 1: field-following horde agent
Level 2: local avoidance and attack behavior
Level 3: elite tactical behavior
Level 4: boss behavior
```

Also include the idea that offscreen hordes may exist as abstract pressure groups and materialize near relevant spaces.

### docs/11_PROJECTILES_BUFFS_PARTICLES.md

Describe the planned event chain:

```txt
ability used
projectile spawned
projectile moves
projectile impacts
damage/buff/particle/sound/noise emitted
horde attention changes
UI/log/minimap update
```

### docs/12_TESTING.md

Define test strategy:

```txt
unit tests
headless simulation tests
deterministic seed tests
event log tests
pathfinding tests later
replay tests later
sanitizer builds
```

### docs/13_KNOWN_ISSUES.md

Start with current known issues:

```txt
SDL3 availability may vary by environment.
No ECS/event bus implemented yet.
No Lua integration yet.
No deterministic replay harness yet.
No generated indexes yet.
Milestone 0 is only foundation.
```

### docs/14_SIMULATION_DOCTRINE.md

Create a doctrine doc around systemic opacity.

It must include:

```txt
The player is not owed omniscience.
The simulation is owed consistency.
The developer is owed traceability.
The agent is owed tests and indexes.
```

Define forensic legibility:

```txt
The UI may not predict every consequence, but after an event chain occurs, the engine should be able to expose enough state, logs, fields, and causal breadcrumbs for debugging and eventual player-facing investigation tools.
```

### docs/15_AGENT_WORKFLOW.md

Explain how future coding agents should pick a ticket, inspect docs, edit, test, and archive a session.

---

## Backlog requirements

Create:

```txt
backlog/active.md
backlog/todo.md
backlog/done.md
backlog/tickets/
```

`backlog/active.md` should point to the current bootstrap task and then the next suggested ticket.

`backlog/todo.md` should list initial ticket IDs grouped by category.

`backlog/done.md` should start empty except for a header.

Create at least these ticket files:

```txt
backlog/tickets/CORE-001-cmake-skeleton.md
backlog/tickets/CORE-002-fixed-timestep-loop.md
backlog/tickets/CORE-003-log-assert-arena-rng.md
backlog/tickets/TOOL-001-clangd-compile-commands.md
backlog/tickets/TOOL-002-build-format-test-scripts.md
backlog/tickets/DOC-001-agent-docs-index.md
backlog/tickets/ECS-001-entity-id-generation-storage.md
backlog/tickets/EVT-001-typed-event-bus-prototype.md
backlog/tickets/WORLD-001-axial-hex-coordinate-conversion.md
backlog/tickets/NAV-001-path-request-handle-api.md
backlog/tickets/NOISE-001-noise-emitted-event-and-field-impulse.md
backlog/tickets/HORDE-001-monster-hearing-and-noise-interest.md
backlog/tickets/COLONY-001-job-entity-and-job-board.md
```

Each ticket must use this template:

```md
# TICKET ID: Title

## Objective

## Context

## Acceptance criteria

## Allowed files

## Out of scope

## Required checks

## Notes
```

For this first run, mark Milestone 0-related tickets as active or ready, but do not falsely mark them done unless actually completed.

---

## Session archive requirement

Create a session archive entry at:

```txt
sessions/YYYY-MM-DD/001-bootstrap-foundation.md
```

Use the actual current date from the environment if available. If not, use:

```txt
sessions/unknown-date/001-bootstrap-foundation.md
```

It must include:

```md
# Session: Bootstrap foundation

## Goal

## Scope

## Files touched

## Decisions

## Known issues

## Tests run

## Next tasks
```

Update it honestly based on what was actually done.

---

## Initial code requirements

### Logging

Implement simple logging with levels:

```txt
TRACE
DEBUG
INFO
WARN
ERROR
FATAL
```

It may print to stderr/stdout. Include file/line macros if practical.

### Asserts

Add project assert macros for debug checks. Keep them simple.

### Arena

Implement a basic linear arena:

```c
typedef struct GameArena {
    unsigned char *base;
    size_t capacity;
    size_t offset;
} GameArena;
```

Functions:

```c
bool game_arena_init(GameArena *arena, size_t capacity);
void game_arena_destroy(GameArena *arena);
void *game_arena_push(GameArena *arena, size_t size, size_t alignment);
void game_arena_reset(GameArena *arena);
```

Include tests for allocation, alignment, reset, and overflow returning `NULL`.

### RNG

Implement deterministic RNG with explicit seed.

Functions may include:

```c
void game_rng_seed(GameRng *rng, uint64_t seed);
uint32_t game_rng_u32(GameRng *rng);
float game_rng_float01(GameRng *rng);
```

Include tests proving the same seed gives the same sequence.

### Fixed timestep

Create a simple fixed timestep skeleton:

```txt
initialize
accumulate time
step simulation at fixed dt
render if SDL target exists
shutdown
```

No gameplay is required.

### Test runner

Implement a minimal C test runner that can run arena and RNG tests.

Tests should return nonzero on failure.

---

## README requirements

README must include:

```txt
project summary
build requirements
how to configure debug build
how to run tests
how to build headless if SDL3 is missing
repo structure
agent workflow summary
current milestone
```

---

## Future architecture to document, not implement yet

Do not implement these now, but document their intended place:

```txt
ECS
event bus
hex world
fog of war
noise/scent/light fields
horde AI
colony jobs
Lua schemas
projectiles/buffs/particles
save/load
navigation service
UI command layer
generated indexes
replay harness
```

---

## Acceptance criteria for this bootstrap run

The run is successful only if:

1. Repository structure exists.
2. CMake config exists.
3. Headless tests build and run, or any failure is clearly documented with exact error output.
4. Core logging/assert/arena/RNG/time files exist.
5. At least arena and RNG tests exist.
6. `AGENTS.md` exists and contains project-specific agent rules.
7. Docs 00 through 15 exist with meaningful initial content.
8. Backlog files and seed tickets exist.
9. Session archive entry exists.
10. README explains how to build and test.
11. No generated files are edited by hand except initial placeholder indexes if explicitly marked as placeholders.
12. The final response summarizes:
    - what was created
    - what builds/tests passed
    - what failed, if anything
    - next recommended ticket

---

## Important boundaries

Do not:

```txt
implement a full ECS in this first run
implement SDL rendering beyond a minimal optional window loop
add Lua dependency unless trivial and available
implement gameplay
create broad fake systems with no tests
hide build failures
claim tests passed if they did not run
edit generated files as if they were source-of-truth
produce only documents and no compiling core code
```

Do:

```txt
build a sturdy foundation
keep code small and plain
prefer headless testability
write docs that future agents can actually use
leave a clear trail
treat the project as a large autonomous simulation that will grow through disciplined tickets
```

Begin now.
