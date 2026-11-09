# Scene and replay formats

Both formats are UTF-8-compatible ASCII, line oriented, canonical when written by the engine, and
explicitly versioned. Parsers reject unknown headers, missing/extra fields, inconsistent counts,
invalid ranges, and non-empty trailing lines.

## Scene version 1

```text
FORGE2D_SCENE 1
bounds <min-x> <min-y> <max-x> <max-y>
entities <count>
entity <name> <x> <y> <vx> <vy> <half-x> <half-y> <rgba> <layer> <player>
```

- Bounds must have positive area and stay within ±10,000,000 simulation units.
- At most 10,000 entities are accepted.
- Names are unique, 1–64 characters, and contain only letters, digits, `_`, or `-`.
- Positions must lie inside scene bounds and half extents must be positive.
- RGBA is an unsigned 32-bit decimal value in `0xRRGGBBAA` semantic order.
- Layer is a signed 16-bit integer; player is `0` or `1`; at most one player exists.
- The demo app additionally caps the source file at one MiB before parsing.

The repository-authored [arena scene](../assets/arena.scene) contains only primitive/color data and
has no external or copyrighted art dependency.

## Scene version 2

```text
FORGE2D_SCENE 2
bounds <min-x> <min-y> <max-x> <max-y>
entities <count>
entity <name> <x> <y> <vx> <vy> <half-x> <half-y> <rgba> <layer> <player> <category> <mask>
```

All v1 constraints remain. Category/mask are unsigned32-bit decimal fields;
category must be a nonzero single bit, mask may be zero. Negative/overflowing,
missing or extra fields fail closed. Version1 inputs remain readable with
category1/all mask bits. Scene writers now emit canonical version2. The
[layer example](../assets/collision-layers.scene) demonstrates two-way inclusion
and an intentionally disabled mask. This affects reported AABB pairs only.

## Replay version 1

```text
FORGE2D_REPLAY 1
seed <unsigned-64-bit>
tick_rate <1..1000>
frames <count>
input <move-x> <move-y> <action>
```

At most one million frames are accepted. Each movement axis is `-1`, `0`, or `1`; action is `0` or
`1`. The seed records provenance for future randomized systems but current world stepping uses no
random number generator.

## Compatibility

Scene writers emit version2 and replay writers version1. Legacy scene readers migrate only the
documented defaults, with no field loss. State hashing includes the two filter words, so old
numeric state hashes are intentionally not compatible with this new fingerprint layout.
A changed grammar or semantic interpretation requires a new
header version and migration test; silently accepting new fields would make replay evidence
ambiguous. Unknown future versions fail closed.
