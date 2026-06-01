# Development Plan: Autonomous Simulation Horde RTS/RPG

This plan defines the staged development path for a large-scope top-down 2D hex-grid horde survival RTS/RPG built in C with SDL3, Lua-driven content, ECS, event bus, colony simulation, navigation services, horde AI, and agent-first engineering.

The project’s tonal and systemic north stars are:

> The spreadsheet under the floor coughed.

and:

> The world hears you.

The first phrase defines the desired systemic feel: the player is one actor inside a vast autonomous world. The second defines the primary fantasy: action creates traces, traces propagate, and the world reacts.

---

## 1. Product identity

The game is a party-first survival RTS/RPG. The player primarily controls an RPG party, but the surrounding world is not passive. Colonists work, monsters investigate, fields decay and propagate, jobs fail, workers panic, nav topology changes, construction makes noise, rituals become epicenters, and offscreen horde pressure can move through the world before materializing near relevant spaces.

The player should feel they are leading an expedition through a machine that was running before they arrived and will continue running after they leave.

Core loop:

```txt
Explore with party
  -> make tactical and logistical decisions
  -> create sound, light, scent, blood, heat, magic, danger, jobs, loot, and combat
  -> autonomous systems react
  -> horde pressure and colony behavior shift
  -> survive, investigate, extract, build, upgrade, and dive deeper
```

The game is not about perfectly forecasting every consequence. It is about living inside a causal system and learning its habits.

---

## 2. Design pillars

### 2.1 Simulation-first autonomy

The world has its own rules. The player is powerful, but not privileged by the simulation. Systems should continue to operate outside direct attention, subject to performance LOD and abstraction.

### 2.2 Forensic legibility, not omniscience

The UI does not need to reveal every consequence in advance. It should provide instruments, logs, stale reports, overlays, reports, pings, scout information, and aftermath investigation tools.

The player is not owed omniscience.
The simulation is owed consistency.
The developer is owed traceability.
The agent is owed tests and indexes.

### 2.3 Noise and attention as a central currency

Noise is danger, information, bait, cost, and leverage. Loud play should not simply be wrong. It should sometimes be worthwhile, sometimes catastrophic, and often uncertain.

Noise exists as:

```txt
one-shot impulse
decaying field
AI memory source
horde attractor
epicenter input
UI/debug trace
possible player tactic
```

### 2.4 Party-first control

The party is the main player body. Colony systems matter, but the player’s emotional perspective is expeditionary: prepare, enter danger, make noise, survive consequences, extract, and return changed.

### 2.5 Large scope through disciplined agents

The scope is intentionally large. Control it with:

```txt
small tickets
allowed file lists
session archives
generated indexes
module docs
headless tests
deterministic seeds
replay hooks
build/test scripts
clear ownership rules
```

---

## 3. Architecture overview

```txt
Platform Layer
  SDL window, input, audio, timing, file access

Core Layer
  memory, logging, math, random, time, asserts, arenas

Input Layer
  raw input -> input actions -> UI/world commands

UI Layer
  HUD, panels, minimap, tooltips, action bars, overlays

Command Layer
  validates player or scripted intent and emits accepted commands/events

ECS Layer
  entities, components, queries, systems, deferred destruction

Event Bus
  typed events, immediate/deferred/audio/debug/persistence queues

World Layer
  hex grid, chunks, terrain, fog, visibility, dungeon maps

Navigation Service
  hex costs, region graph, flow fields, path cache, spatial indices, steering

Simulation Layer
  party, combat, horde AI, workers, colony, projectiles, buffs, particles, fields

Lua/Data Layer
  content definitions, behavior definitions, schema validation, generated loaders

Render Layer
  world rendering, sprite batching, particles, fog, UI drawing

Audio Layer
  audible sound playback plus simulated noise events

Agent Harness
  backlog, docs, generated indexes, session archives, build/test/LSP tools
```

Primary rule:

```txt
UI emits intent.
Command system validates intent.
Simulation changes state.
Events report results.
UI reflects state.
```

---

## 4. Technical baseline

```txt
Language: C17
Platform: SDL3
Build: CMake
Compiler preference: Clang
LSP: clangd with compile_commands.json
Data/scripting: Lua
Testing: C test runner, headless simulation tests, deterministic seed tests
Formatting: clang-format
Static analysis: clang-tidy where available
Debugging: logs, event dumps, ECS stats, replay hooks, debug overlays
```

Headless simulation and tests must remain usable without SDL.

---

## 5. Repository lanes

The repository has three long-term lanes.

### 5.1 Runtime lane

The actual game code:

```txt
src/core
src/platform
src/ecs
src/event
src/world
src/nav
src/sim
src/game
src/colony
src/lua
src/script
src/render
src/audio
src/ui
```

### 5.2 Content lane

Data, schemas, generated loaders, validation:

```txt
data/lua
schema
generated
tools/generators
tools/validators
```

### 5.3 Agent lane

Scaffolding that keeps long-term agent work sane:

```txt
docs
backlog
sessions
tools/indexers
tools/agent_tools
scripts
tests
```

---

## 6. Major system plans

### 6.1 ECS

Rules:

```txt
Entities are handles with generation counters.
Components are plain data.
Systems own behavior.
Component storage is contiguous where possible.
Entity destruction is deferred until safe points.
Queries should be explicit and testable.
```

Initial target:

```c
typedef struct Entity {
    uint32_t index;
    uint32_t generation;
} Entity;
```

Do not let ECS become occult plumbing. Generated indexes should eventually answer:

```txt
What components exist?
What systems read/write each component?
What events are produced/consumed?
What tests cover the system?
```

### 6.2 Event bus

Rules:

```txt
Commands are requests.
Events are facts.
Direct scheduling handles deterministic core simulation.
Events carry cross-system consequences, UI/audio/debug reactions, delayed effects, and causal traces.
```

Planned queues:

```txt
immediate_bus
  same-tick gameplay consequences

deferred_bus
  end-of-tick changes, destruction, delayed state updates

audio_bus
  playback requests

debug_bus
  telemetry, overlays, event tracing

persistence_bus
  save/load relevant scheduled events
```

Important early events:

```txt
EV_NOISE_EMITTED
EV_ENTITY_DAMAGED
EV_ENTITY_DIED
EV_ABILITY_USED
EV_PROJECTILE_SPAWNED
EV_PROJECTILE_IMPACT
EV_BUFF_APPLIED
EV_BUFF_EXPIRED
EV_PARTICLE_REQUESTED
EV_DOOR_BROKEN
EV_LOOT_DROPPED
EV_DUNGEON_ROOM_REVEALED
EV_EPICENTER_CREATED
EV_EXTRACTION_STARTED
EV_WAVE_ESCALATED
EV_WORK_ORDER_CREATED
EV_JOB_ASSIGNED
EV_JOB_FAILED
EV_STRUCTURE_BUILT
EV_STRUCTURE_DESTROYED
EV_NAV_TOPOLOGY_CHANGED
EV_COMMAND_ACCEPTED
EV_COMMAND_REJECTED
```

### 6.3 Hex world

Use axial coordinates as authoritative gameplay space.

```c
typedef struct Hex {
    int q;
    int r;
} Hex;
```

Derive cube coordinate `s` when needed:

```txt
s = -q - r
```

Early cell data:

```txt
terrain_type
flags
movement_cost
light_level
noise_value
scent_value
danger_value
height
room_id
fog_state
```

Chunking supports visibility, fields, horde activation, path invalidation, render culling, and job lookup.

### 6.4 Navigation

Layer order:

```txt
1. Hex grid
   authoritative gameplay space

2. Region/portal graph
   high-level routing across rooms, chunks, gates, corridors, bridges

3. Path request budget
   systems submit path requests and receive handles/status

4. Flow fields
   hordes, evacuations, rally movement, repeated movement toward epicenters

5. Spatial index
   lookup nearby agents, sounds, jobs, items, threats, obstacles

6. Local steering
   separation, cohesion, crowd pressure, local avoidance

7. Navmesh
   later only if proven necessary
```

Do not begin with navmesh. The hex grid is authoritative.

### 6.5 Noise, fields, and attention

Noise should eventually include:

```txt
impulse events
bounded propagation or diffusion
terrain dampening
door/wall occlusion
field decay
monster hearing queries
horde attention/memory
minimap pings
logs/debug traces
```

Other fields may include:

```txt
scent
light
danger
heat
blood
magic resonance
influence
```

Avoid implementing every field at once. Use the same conceptual pipeline but start with noise.

### 6.6 Horde AI

Layered AI:

```txt
Level 0: dormant or abstract offscreen pressure
Level 1: field-following horde agent
Level 2: local avoidance and attack behavior
Level 3: elite tactical behavior
Level 4: boss behavior
```

Offscreen hordes can exist as abstract pressure groups. They materialize as individual agents when near visible or tactically relevant spaces.

Monster interest states:

```txt
Idle
HeardSomething
Investigating
ConfirmedTarget
Attacking
Searching
GivingUp
ReturningOrDispersing
```

Performance rules:

```txt
Do not pathfind every monster.
Do not update every AI every frame.
Use flow fields for common goals.
Use density maps to avoid stacking.
Use staggered updates.
Use chunk activation and LOD.
Use group controllers for far-away hordes.
```

### 6.7 Colony and jobs

Colony systems are first-class but should be implemented in layers.

Core concepts:

```txt
Work order
  player/system-level desired outcome

Job
  executable unit of work

Reservation
  temporary claim on job, item, station, bed, stockpile slot, or target

Role
  worker policy and priority profile

Behavior
  scripted sequence/tree of actions used to execute a job
```

Threat states:

```txt
NORMAL
WATCH
ALERT
SIEGE
PANIC
SILENT
```

Initial jobs:

```txt
haul material
build barricade
repair barricade
flee/shelter under threat
```

Later jobs:

```txt
craft
mine
farm
doctor
guard
operate station
ritual support
logistics
construction chains
```

### 6.8 Lua and data

Lua defines:

```txt
items
weapons
armor
abilities
projectiles
buffs
particles
monsters
jobs
worker roles
behavior trees
terrain types
buildings
loot tables
dungeon rooms
UI action metadata
tooltips
```

C owns:

```txt
ECS memory
hot simulation loops
pathfinding
flow fields
steering
visibility
render batching
audio playback
event bus
save/load core
deterministic rules
```

Boundary rule:

```txt
Lua defines data and high-level behavior.
Lua does not directly mutate arbitrary ECS memory.
Lua requests actions through a narrow script API.
```

### 6.9 UI and UX

UI philosophy:

```txt
The UI provides instruments, not omniscience.
```

Planned UI:

```txt
top status bar
world view
minimap
bottom RPG panel
party portraits
action bar
inventory/character panels
colony panel
build panel
logs
field overlays
noise meter
sound pings
camera leash
```

The UI should support forensic legibility. It may not forecast every consequence, but after something happens, the game should have enough logs and state to explain or hint at why.

### 6.10 Projectiles, buffs, particles

Planned event chain:

```txt
Ability used
  -> EV_ABILITY_USED
  -> projectile spawned
  -> projectile moves
  -> projectile impacts
  -> EV_PROJECTILE_IMPACT
  -> damage applied
  -> buff applied
  -> particle burst requested
  -> sound requested
  -> noise emitted
  -> horde attention changes
  -> UI log/minimap pulse updates
```

These should be data-driven early enough to support iteration, but not before the core ECS/event/data foundations exist.

---

## 7. Milestone roadmap

### Milestone 0: Project Skeleton and Agent Foundation

Goal: Create the repository foundation.

Deliverables:

```txt
CMake project
SDL optional app target
headless test target
fixed timestep skeleton
logging
asserts
basic arena
seeded RNG
time utilities
compile_commands.json
clang-format config
clang-tidy config
build/test/format scripts
AGENTS.md
docs 00-15
backlog seed tickets
session archive
README
```

Quality gate:

```txt
Headless tests configure, build, and run.
Arena and RNG tests pass.
Docs and backlog exist.
```

### Milestone 1: Core Runtime Loop and Diagnostics

Goal: Make the runtime observable and deterministic enough for future systems.

Deliverables:

```txt
improved fixed timestep
simulation tick counter
seeded global simulation context
log channels
eventual debug flag structure
basic profiler/timing scopes
headless run mode
```

Quality gate:

```txt
Headless simulation runs N ticks with deterministic RNG output.
Diagnostics can be dumped to a log.
```

### Milestone 2: ECS and Event Bus

Goal: Establish entity/component/event foundation.

Deliverables:

```txt
entity handles with generation counters
entity allocation/destruction
basic component storage
system schedule skeleton
typed event definitions
immediate/deferred event queues
command/event distinction in docs and tests
```

Quality gate:

```txt
Tests cover entity generation reuse, component add/remove, event push/pop, event ordering, and queue overflow behavior.
```

### Milestone 3: Hex World and Visibility Storage

Goal: Create authoritative world space.

Deliverables:

```txt
axial hex coordinates
hex neighbor/distance/ring helpers
hex/world conversion
hex map storage
chunk storage prototype
terrain costs
fog state storage
```

Quality gate:

```txt
Tests cover coordinate math, neighbor consistency, distance, map lookup, and chunk boundaries.
```

### Milestone 4: Platform Window, Camera, and Debug Rendering

Goal: See the world.

Deliverables:

```txt
SDL window path
camera transform
hex debug draw
camera pan/zoom/focus
party anchor placeholder
basic debug overlays
```

Quality gate:

```txt
Headless tests still work without SDL.
SDL app shows a hex grid if SDL3 is available.
```

### Milestone 5: Command Layer and Input Routing

Goal: Prevent UI from mutating simulation directly.

Deliverables:

```txt
input action map
interaction modes
command structs
command validation skeleton
command accepted/rejected events
right-click move placeholder
```

Quality gate:

```txt
Tests cover valid/invalid command validation in headless mode.
```

### Milestone 6: Party Control Foundation

Goal: The player has a body.

Deliverables:

```txt
party member entities
position component
health component
faction component
selection state
party centroid/leader anchor
basic movement intent
bottom panel shell later if UI exists
```

Quality gate:

```txt
Party entities can be created, selected, commanded, and simulated in headless tests.
```

### Milestone 7: Navigation Service v1

Goal: Budgeted pathfinding and route ownership.

Deliverables:

```txt
navigation profiles
path request struct
path handle API
budgeted hex A*
path statuses
path cache seed
nav epoch
nav topology changed event
```

Quality gate:

```txt
Tests cover reachable path, blocked path, partial/failed status, budget behavior, and invalidation.
```

### Milestone 8: Noise Events and Field Impulses

Goal: Make the world hear.

Deliverables:

```txt
EV_NOISE_EMITTED
noise categories
noise impulse propagation prototype
terrain dampening
field decay
noise debug dump
basic monster hearing query API
```

Quality gate:

```txt
Tests cover impulse radius, dampening, decay, and deterministic propagation.
```

### Milestone 9: First Monster Hearing

Goal: Prove autonomous reaction to noise.

Deliverables:

```txt
monster entity
hearing component
interest state
noise memory
movement toward heard sound
simple attack/contact behavior
```

Quality gate:

```txt
Headless scenario: noisy party action causes monster to investigate. Silent action does not.
```

### Milestone 10: UI Instruments v1

Goal: Build instruments, not omniscience.

Deliverables:

```txt
top status bar shell
bottom RPG panel shell
logs panel
noise meter
field overlay toggle
minimap shell
stale information styling concept
```

Quality gate:

```txt
UI reflects simulation state through commands/events, not direct mutation.
```

### Milestone 11: Lua Data Pipeline v1

Goal: Load content safely.

Deliverables:

```txt
Lua VM initialization
content registry
schema validation prototype
ability/projectile/buff definition examples
generated or hand-authored loaders v1
content error reporting
```

Quality gate:

```txt
Tests cover valid content load, invalid content rejection, duplicate ID handling, and deterministic registry IDs.
```

### Milestone 12: Projectiles, Damage, Buffs, and Particles v1

Goal: Establish ability consequence chains.

Deliverables:

```txt
projectile component
projectile movement
projectile impact event
damage application
buff apply/expire
particle request event
sound/noise payload fields
```

Quality gate:

```txt
Headless test: ability spawns projectile, projectile impacts, damage applies, noise emits, event log records chain.
```

### Milestone 13: Horde AI v1

Goal: Move from single monsters to group pressure.

Deliverables:

```txt
horde group controller
abstract offscreen pressure
materialization near relevant space
flow field toward noise/epicenters
local density map
staggered updates
```

Quality gate:

```txt
Stress test hundreds of agents.
Abstract group can move toward an epicenter and materialize deterministically.
```

### Milestone 14: Colony Jobs v1

Goal: Make the base act.

Deliverables:

```txt
work orders
jobs as entities
job board
worker entities
assignment scoring
reservations
haul job
build barricade job
repair job
threat state modifiers
```

Quality gate:

```txt
Headless test: worker reserves job/resources, completes build, releases reservation, emits construction noise.
```

### Milestone 15: Construction, Stockpile, and Nav Invalidation

Goal: Colony changes the world.

Deliverables:

```txt
stockpile inventory
blueprint placement
walls/doors/barricades
construction stages
construction noise
EV_NAV_TOPOLOGY_CHANGED
dirty nav regions
```

Quality gate:

```txt
Building a barricade changes movement cost/blockage, invalidates relevant paths, and produces noise.
```

### Milestone 16: Dungeon Prototype

Goal: Expedition loop in a contained environment.

Deliverables:

```txt
dungeon map generation or authored test map
rooms/corridors/doors
noise occlusion
sleeping enemies
loot room
extraction point
fog/minimap integration
```

Quality gate:

```txt
Party enters dungeon, makes noise, monsters react, party extracts loot.
```

### Milestone 17: Threat-Aware Colony

Goal: Colony responds to danger.

Deliverables:

```txt
NORMAL/WATCH/ALERT/SIEGE/PANIC/SILENT states
workers avoid dangerous jobs
shelter/flee behavior
guard post placeholder
silent policy suppresses noisy jobs
emergency repair priority
```

Quality gate:

```txt
Threat state changes job scoring and worker behavior in deterministic tests.
```

### Milestone 18: Integrated Vertical Slice

Goal: Prove the whole identity.

Scenario:

```txt
The party enters a small dungeon.
They recover a relic.
Opening doors, fighting, and abilities create noise.
Monsters and horde pressure react.
The party escapes to the base.
Workers repair/build a barricade.
Construction creates more noise.
The player survives, extracts value, and sees aftermath traces.
```

Quality gate:

```txt
The player can complete a short run with party control, dungeon danger, noise consequences, horde response, colony repair/building, and visible forensic traces.
```

---

## 8. Ticket categories

Use these categories in backlog:

```txt
Foundation
ECS/Event Bus
World/Hex
Navigation
Noise/Fields
Horde AI
UI/UX
Lua/Data
Projectiles/Buffs/Particles
Party/RPG
Colony/Jobs
Dungeon
Testing/Tools
Performance
Known Issues
```

Seed ticket examples:

```txt
CORE-001: Create CMake skeleton with optional SDL target
CORE-002: Add fixed timestep loop
CORE-003: Add logging/assert/memory arena/RNG baseline
TOOL-001: Generate compile_commands.json and clangd setup
TOOL-002: Add format/build/test scripts
DOC-001: Create docs index and agent directives
ECS-001: Entity ID and generation storage
ECS-002: Component registration prototype
EVT-001: Typed event bus prototype
UI-001: UI context, button, label, panel
UI-002: Top status bar shell
UI-003: Bottom RPG panel shell
WORLD-001: Axial hex coordinate conversion
WORLD-002: Hex map and chunk storage
CAM-001: Camera leash around party anchor
NAV-001: Path request handle API
NAV-002: Basic hex A* under request budget
LUA-001: Lua VM initialization
LUA-002: Lua schema validation prototype
SIM-001: Projectile component and movement
SIM-002: Projectile impact event
SIM-003: Buff application system
SIM-004: Particle request event
NOISE-001: Noise emitted event and field impulse
HORDE-001: Monster hearing and movement toward noise
COLONY-001: Job entity and job board
COLONY-002: Worker assignment scoring
COLONY-003: Reservation system
```

Ticket template:

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

Tickets must be small enough that an agent can finish, test, and archive one without broad wandering.

---

## 9. Quality gates

Every milestone should define:

```txt
what changed
what files/modules are involved
what tests prove it
what debug output exists
what remains intentionally out of scope
```

Minimum recurring checks:

```sh
./scripts/configure_debug.sh
./scripts/build_debug.sh
./scripts/test.sh
```

Additional checks when available:

```txt
clang-format
clang-tidy
sanitizers
headless replay
benchmark/stress test
LSP diagnostics
```

Never claim a check passed if it was not run.

---

## 10. Agent workflow

Each agent session should follow this sequence:

```txt
1. Read AGENTS.md.
2. Read docs/00_INDEX.md.
3. Read docs/01_AGENT_DIRECTIVES.md.
4. Read backlog/active.md.
5. Pick exactly one ready ticket unless instructed otherwise.
6. Read module-specific docs.
7. Search existing names and patterns.
8. Edit only allowed files unless the ticket is defective.
9. Add or update tests.
10. Format changed files.
11. Build.
12. Run targeted tests.
13. Update docs if architecture changed.
14. Update backlog status.
15. Add session archive entry.
16. Summarize honestly.
```

Recurring agent prompt:

```md
Read AGENTS.md, docs/00_INDEX.md, docs/01_AGENT_DIRECTIVES.md, backlog/active.md, and the next ready ticket. Work exactly one ticket. Stay within allowed files unless the ticket is defective. Build, test, update the session archive, and summarize results honestly.
```

---

## 11. Documentation strategy

Use progressive disclosure.

Default session context:

```txt
AGENTS.md
docs/00_INDEX.md
docs/01_AGENT_DIRECTIVES.md
current backlog ticket
module-specific doc
recent session archive if relevant
```

Do not dump the full design bible into every coding session. Keep module docs concise and navigable.

Generated indexes should eventually include:

```txt
component_index.md
event_index.md
system_index.md
lua_schema_index.md
module_dependency_graph.md
public_headers_index.md
test_index.md
```

---

## 12. Performance principles

Do not:

```txt
pathfind every monster
update every AI every frame
allocate in hot loops
send events for every micro-movement
make every system scan every entity
let UI mutate simulation state directly
```

Do:

```txt
use chunking
use contiguous component arrays
use fixed timestep simulation
use flow fields for mass movement
use spatial indices for retrieval
use path request budgets
use debug overlays
profile from the start
use deterministic tests
```

Optimization order:

```txt
1. correct data ownership
2. deterministic behavior
3. observability
4. simple performance budgets
5. profiling
6. targeted optimization
7. SIMD/vectorization only after measurement
```

---

## 13. Risk register

### Risk: event bus becomes hidden control-flow soup

Mitigation:

```txt
events are facts, not requests
generated producer/consumer index
event ordering docs
tests for event queue behavior
```

### Risk: agent sessions make broad unreviewable changes

Mitigation:

```txt
small tickets
allowed files
session archives
required checks
backlog status updates
```

### Risk: simulation opacity feels arbitrary

Mitigation:

```txt
forensic logs
causal event chains
debug overlays
field dumps
aftermath reports
consistent rules
```

### Risk: pathfinding/horde performance collapses

Mitigation:

```txt
flow fields
abstract offscreen pressure groups
LOD
chunk activation
staggered updates
path request budgets
stress tests
```

### Risk: Lua boundary becomes unsafe

Mitigation:

```txt
schema validation
narrow script API
no arbitrary ECS mutation
C owns hot loops
content registry tests
```

### Risk: colony sim swallows project before expedition loop works

Mitigation:

```txt
jobs are first-class but staged
start with haul/build/repair/flee
connect construction noise to horde pressure early
avoid full needs/schedules until later
```

---

## 14. First integrated scenario target

The first integrated vertical slice should contain:

```txt
one base/overworld map
one dungeon entrance
two to four party members
one worker role: Builder/Hauler hybrid
one stockpile
one buildable barricade
one monster type
one projectile ability
one buff
one particle request
one noise field
one extraction objective
one horde response
```

Scenario:

```txt
The party enters a small dungeon.
They recover a relic.
Using abilities and opening doors creates noise.
Monsters begin flowing toward the party.
The party escapes to the base.
Workers repair/build a barricade.
Construction creates more noise.
The player survives, extracts, and reads aftermath traces.
```

This tests the identity of the whole game without requiring the full final game.

---

## 15. Long-term target loop

```txt
Player gives orders through UI.
  -> command system validates intent
  -> party acts, workers act, abilities fire, jobs execute
  -> events are emitted
  -> noise, scent, light, combat, particles, and nav changes update the world
  -> monsters and abstract horde groups flow toward activity
  -> colony reacts through jobs and threat states
  -> UI provides instruments, stale reports, logs, and forensic traces
  -> RPG party survives, grows, extracts, and returns deeper
```

Everything should reinforce that loop.
