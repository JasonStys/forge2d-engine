# Architecture

## Goals

- Make simulation deterministic enough for exact local replay and regression hashes.
- Keep core logic testable without graphics hardware or an event loop.
- Show cache-conscious component storage, bounded memory behavior, algorithm choices, and error
  handling in a reviewable scope.
- Support Windows, Linux, and macOS toolchains through ordinary CMake targets.

## Non-goals

Networking, continuous rigid-body physics, a general editor, scripting, 3D rendering, and a
production asset pipeline are intentionally excluded.

## Component model

```text
platform adapter                       dependency-free core
----------------                       --------------------
wall-clock duration -> FixedStepClock -> World::step(InputFrame)
keyboard/controller -> InputFrame     -> EntityPool + SparseSet<T>
SDL3 renderer       <- sorted snapshot <- integer Transform/KinematicBody/Sprite
headless PPM        <- sorted snapshot <- SpatialHash exact pairs
scene/replay text   -> strict parser   -> stable state/pixel hashes
                                          A* grid pathfinding
                                          C17 bounded arena
```

`World` is the simulation aggregate. Entities are 64-bit index/generation handles. Each component
type has a dense entity/component vector and sparse index vector. The world advances positions in
integer subpixels per tick and clamps/bounces bodies at world boundaries. Snapshot order is layer,
then entity ID, so render output and hashes do not depend on container implementation details.

## Data flow

1. A scene parser validates a versioned manifest and produces data-only `SceneEntity` records.
2. `World::from_scene` allocates handles and inserts three component types.
3. A platform adapter converts elapsed time to bounded tick count and device state to `InputFrame`.
4. `World::step` applies player intent and kinematic movement without reading time or input APIs.
5. Spatial hash maps AABBs into integer cells, creates candidate pairs, applies exact overlap, and
   returns sorted unique pairs.
6. Snapshot and explicit little-endian-style hashing provide regression evidence.
7. Software or SDL rendering consumes snapshots but cannot mutate simulation state.

## Ownership and lifetime

`World` owns component vectors by value. No system retains pointers across insert/erase. Entity
generations invalidate stale handles after reuse. `SdlRenderer` exclusively owns its `SDL_Window`
and `SDL_Renderer`; copying and moving are disabled. The C arena never owns its backing bytes.

## Failure behavior

- Invalid world/grid/image/clock configuration throws before partial use.
- Invalid scene/replay data throws `std::invalid_argument`; oversized files are rejected by apps.
- Missing components throw on direct `get`, while `contains`/`erase` support explicit branching.
- Arena failure returns null without mutation.
- Excess wall-clock backlog is dropped and counted rather than creating unbounded catch-up work.

## Extensibility seams

New components can use `SparseSet<T>`; systems should consume explicit world/view interfaces rather
than global state. New presentation backends consume immutable snapshots. A future asset pipeline
should create a new versioned format rather than relaxing this parser. Collision response should be
a separate narrowphase/solver layer, leaving broadphase oracle tests intact.

See [ADR 0001](adr/0001-headless-core-and-optional-sdl3.md) for the renderer/dependency decision.
