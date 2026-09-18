# Benchmark report

## Local MSVC result — 2026-09-18

Environment: Windows, MSVC 19.51.36257, x64 Debug build for this initial local measurement. Release
CI results are retained as workflow artifacts and are expected to be faster.

| Scenario | Workload | Result | Budget | Status |
|---|---:|---:|---:|---|
| Sparse-set lifecycle | create/update/half erase for 20,000 entities | 10.3671 ms | 1,000 ms | pass |
| Spatial broadphase | 20,000 70×70 AABBs in 100,000² world | 55.8107 ms | 2,500 ms | pass |
| Fixed simulation | 5,000 entities × 240 ticks | 119.921 ms | 2,500 ms | pass |

The broadphase produced 383 exact intersecting pairs. All timings use `steady_clock` around only the
named workload. They exclude process startup and JSON output.

## Interpretation

The generous ceilings detect large complexity/regression changes while tolerating shared runners.
They are not frame-rate, latency, console/mobile, power, or production capacity claims. Compare
same-compiler/same-configuration artifacts before drawing performance conclusions.

Run `forge2d_benchmark`; it emits JSON and returns nonzero when any budget fails.
