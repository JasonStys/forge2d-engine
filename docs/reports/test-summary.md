# Test summary

## Local verification — 2026-09-18

| Check | Result |
|---|---|
| MSVC compile | passed with `/W4 /WX /permissive- /sdl` |
| Deterministic test executable | 8/8 groups passed |
| CTest | 1/1 executable passed in 3.07 seconds |
| Headless integration demo | passed; 360 ticks, 8 entities, 1 collision |
| State hash | `2140247359617279822` |
| Pixel hash | `14365382764149795246` |
| C arena high-water | 512 bytes |
| SDL3 3.4.16 adapter | compiled successfully from verified official source |
| Clang-tidy 22.1.3 | passed selected analyzer, bug-prone, performance, portability, and readability checks |
| Benchmark budgets | all passed |

The eight groups cover math/fixed-step bounds, randomized sparse-set equivalence, generation reuse,
C arena alignment/overflow/reset, bounded spatial hash versus brute-force oracle, overflow-safe
coordinate arithmetic, world-containment validation, A* path properties,
scene/replay strict round trips, independent world/replay/render determinism, and profiler bounds.

The final hosted run passed GCC/Clang/MSVC Release builds, 88.75% line and 72.18% meaningful branch
coverage, ASan/UBSan, libFuzzer, Linux SDL compilation, packaging, container smoke, repository
validation, clang-tidy, and CodeQL. See [validation.md](validation.md) for immutable run links.
