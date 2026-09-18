# Complexity and performance

Let `n` be live entities, `c` entities with one component type, `p` collision candidate pairs,
`k` occupied spatial cells, `V/E` pathfinding vertices/edges, `f` replay frames, and `w×h` pixels.

| Operation | Time | Additional space | Notes |
|---|---:|---:|---|
| Entity create/destroy/valid | amortized O(1) / O(1) / O(1) | O(n) total | intrusive free-index list plus generation/alive arrays |
| Sparse-set contains/get | O(1) | O(index range) sparse | direct sparse-to-dense index |
| Sparse-set insert/erase | amortized O(1) | O(c) dense | swap-and-pop invalidates order/references |
| World movement tick | O(n) | O(1) | one pass over dense bodies |
| Snapshot | O(n log n) | O(n) | stable layer/entity sorting |
| Spatial-hash broadphase | expected O(n + k + p) | O(n + k + p) | capped at 100,000 items and 4,096 cells/item; degrades toward O(n²) when one cell is crowded |
| Brute-force oracle | O(n²) | O(p) | tests/reference only |
| A* | O((V+E) log V) | O(V) | binary heap; four-neighbor grid |
| Scene/replay parse | O(input bytes) | O(n) / O(f) | explicit count/allocation caps |
| State hash | O(n log n) | O(n) | includes sorted snapshot cost |
| Software render | O(w·h + drawn pixels) | O(w·h) | clear plus clipped fills |
| Arena allocate/reset | O(1) | O(1) metadata | caller owns backing bytes |

## Data locality

Component payloads are contiguous by type, supporting sequential system passes and fewer unrelated
cache lines than pointer-rich per-entity objects. Sparse indices trade memory proportional to the
largest entity index for direct lookup. Swap-and-pop avoids shifting but means callers must not rely
on dense order or retain references across mutation.

## Benchmark method

The benchmark measures a 20,000-entity sparse lifecycle, spatial collision over 20,000 small AABBs,
and 240 ticks over 5,000 entities. Generous ceilings are regression tripwires, not hardware-neutral
throughput promises. See [benchmark.md](reports/benchmark.md) for exact local evidence.
