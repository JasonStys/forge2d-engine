# Public API

Public headers live under `include/forge2d`. The installed CMake targets are `Forge2D::Core` and the
exported C arena target. Exact declarations are in [code-index.md](code-index.md).

## Entity and components

- `Entity` contains a 32-bit index and generation and exposes a stable `packed()` key.
- `EntityPool::create/destroy/valid` manages handle lifetime without owning components.
- `SparseSet<T>` offers `insert_or_assign`, `erase`, `contains`, `get`, dense entity/component
  arrays, and size. Insertions can invalidate component references; do not retain them.
- `Transform`, `KinematicBody`, and `Sprite` are data-only components.
- `InputFrame` is the platform-independent tick input contract with axes in `[-1, 1]`.

## Simulation

`World` owns lifecycle and components:

```cpp
forge2d::World world{{0, 0}, {10'000, 10'000}};
const auto player = world.spawn(
    {{1'000, 1'000}}, {{0, 0}, {200, 200}}, {0x00FF00FFU, 10}, true);
world.step({1, 0, false});
const auto digest = world.state_hash();
```

Only one player is permitted. Bodies require positive half extents and must fit wholly within the
world. `step` rejects axes outside the contract, uses wide intermediate arithmetic before clamping,
performs exactly one simulation tick, and never reads a clock. `snapshot()` returns copies ordered
by sprite layer/entity. `collision_pairs()` returns normalized sorted exact overlaps.

## Timing

`FixedStepClock::consume(elapsed)` converts wall time to zero or more ticks, limited by the configured
per-frame maximum. Excess ticks are dropped and counted. `interpolation_alpha()` exposes the
remaining fraction for presentation interpolation; interpolation does not mutate simulation.

## Collision

`SpatialHash(cell_size)::query(items)` performs broadphase candidate generation and exact AABB
filtering. `brute_force_collisions(items)` is the O(n²) reference oracle. Edge contact alone is not
penetration. Cell size and item half extents must be positive. Queries accept at most 100,000 items,
and each item may occupy at most 4,096 cells, preventing accidental unbounded broadphase expansion.

## Pathfinding

`Grid` is bounded to 4,096 per dimension and four million total cells. `find_path` returns an
inclusive start-to-goal path or an empty vector for invalid/unreachable endpoints. Neighbors are
axis-aligned; stable tie-breaking makes identical inputs return the same path.

## Scene and replay

`parse_scene/serialize_scene` and `parse_replay/serialize_replay` implement strict version-one text
formats. See [asset-format.md](asset-format.md). `run_replay` takes a world by value and returns the
final state hash without mutating the caller's world.

## Rendering

`Image` owns a bounded RGBA framebuffer, supports clipped solid rectangles, writes binary PPM, and
provides a stable pixel hash. `render_world` consumes a snapshot. `SdlRenderer` is available only
when `FORGE2D_BUILD_SDL=ON` and owns the SDL window/renderer.

## C arena

```c
uint8_t storage[4096];
forge2d_arena arena;
forge2d_arena_init(&arena, storage, sizeof storage);
void* block = forge2d_arena_allocate(&arena, 256, 64);
forge2d_arena_reset(&arena);
```

Backing memory remains caller-owned. Allocation is zeroed, aligned, constant-time, and never falls
back to the heap. Null/zero/invalid alignment/overflow/capacity failures return `NULL` without
changing offset. `peak` survives reset.
