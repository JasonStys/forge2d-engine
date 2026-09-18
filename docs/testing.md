# Testing strategy

## Risk model

The highest risks are stale entity handles, sparse/dense divergence, memory/alignment overflow,
collision misses/duplicates, non-optimal or unstable paths, parser allocation abuse, nondeterministic
replay, and platform-only build failures. Tests target those behaviors rather than framework code.

## Test layers

| Layer | Evidence |
|---|---|
| Unit | vector/AABB semantics, fixed-step backlog, arena alignment/overflow, profiler window |
| Model/property | 8,000 randomized sparse-set operations compared with `unordered_map` |
| Algorithm oracle | 150 seeded random collision scenes compared with O(n²) reference |
| Pathfinding | valid adjacency, obstacle avoidance, known shortest route, unreachable route |
| Parser/negative | canonical scene/replay round trips and malformed/header/range rejection |
| Integration | scene → world → 300-tick replay → state hash → software pixel hash |
| Fuzz | coverage-guided scene parser with ASan/UBSan and seed corpus |
| Cross-platform | MSVC, GCC, Clang warnings-as-errors; pinned SDL3 compilation |
| Non-functional | clang-tidy, coverage gates, executable budgets, install/package, container, CodeQL |

## Local commands

```bash
cmake -S . -B build/dev -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/dev --parallel
ctest --test-dir build/dev --output-on-failure
./build/dev/forge2d_benchmark
```

Sanitizers and fuzzing on Clang:

```bash
CC=clang CXX=clang++ cmake -S . -B build/fuzz -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DFORGE2D_ENABLE_SANITIZERS=ON -DFORGE2D_BUILD_FUZZ=ON
cmake --build build/fuzz --parallel
ctest --test-dir build/fuzz --output-on-failure
./build/fuzz/forge2d_scene_fuzz -max_total_time=60 -timeout=2 fuzz/corpus
```

## Coverage

CI instruments GCC builds and uses gcovr 8.6. Required minimums are 80% line and 70% branch over
`include/`, `src/`, and `platform/`. Generated/dependency/build code is excluded. Coverage is a
backstop: reference-equivalence, negative parsing, sanitizer, and fuzz evidence address risks that a
single percentage cannot express.

## Performance stability

Budgets are deliberately much slower than a healthy run to catch algorithmic regressions without
flaking on shared runners. The benchmark exits nonzero above any ceiling; each compiler's JSON and
headless output are uploaded as workflow artifacts.

## Current result

See [test-summary.md](reports/test-summary.md) and [validation.md](reports/validation.md). Cloud-only
coverage, Linux sanitizer/fuzz, GCC/Clang, and container results are finalized by the first Actions
run for each commit.
