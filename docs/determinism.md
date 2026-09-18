# Determinism and replay

## Guarantees in scope

For the same compiled implementation, scene, replay frames, and tick count, simulation state and
software-rendered pixels are repeatable. The test suite runs two independent worlds, compares each
final state hash, reruns through `run_replay`, and compares pixel hashes.

The design removes common accidental nondeterminism:

- integer positions/velocities instead of floating-point simulation;
- exactly one state transition per explicit tick;
- stable A* neighbor order and priority tie-breaking;
- collision pairs normalized, deduplicated, and sorted;
- render snapshots sorted by layer then entity handle;
- explicit byte-order-independent integer feeding into FNV-1a-style regression hashes;
- versioned inputs with exact frame counts and rejected trailing data.

## Fixed timestep

Presentation code submits elapsed nanoseconds to `FixedStepClock`. It emits at most eight ticks by
default and counts/drops older backlog to prevent a slow frame from creating unbounded future work.
The world sees only `InputFrame`, never wall time. Rendering may use interpolation alpha later, but
interpolation must not feed back into canonical state.

## What is not guaranteed

The hash is a regression checksum, not a cryptographic signature. Cross-compiler/network lockstep
is not claimed until the compiler matrix compares golden hashes on every supported target. Future
floating-point physics, nondeterministic jobs, platform math, random systems, or unordered iteration
would require an explicit determinism review.

## Replay drill

```bash
./build/dev/forge2d_arena assets/arena.scene out/first
./build/dev/forge2d_arena assets/arena.scene out/second
```

Compare the printed `state_hash` and `pixel_hash`, and compare the two replay files byte for byte.
The checked local MSVC result is documented in [validation.md](reports/validation.md).
