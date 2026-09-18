# File catalog

This catalog gives reviewers a one-line responsibility for every committed file. Build products,
coverage output, generated demo evidence, and downloaded dependencies are intentionally ignored.
For declaration-level locations, see the generated [code index](code-index.md).

## Project and automation

| File | Responsibility |
|---|---|
| `.clang-format` | Defines the repository's deterministic C/C++ formatting policy. |
| `.clang-tidy` | Records the intended static-analysis checks and treats findings as errors. |
| `.dockerignore` | Keeps generated output, VCS metadata, and editor state out of Docker build contexts. |
| `.editorconfig` | Aligns whitespace, line endings, and final-newline behavior across editors. |
| `.gitattributes` | Normalizes text files while preserving binary PPM output behavior. |
| `.gitignore` | Excludes local builds, coverage, IDE state, and generated runtime evidence. |
| `.github/dependabot.yml` | Requests bounded weekly GitHub Actions and Docker dependency update reviews. |
| `.github/workflows/ci.yml` | Runs cross-platform builds, tests, coverage, sanitizers, fuzz smoke, SDL, packaging, repository validation, and container checks. |
| `.github/workflows/codeql.yml` | Runs GitHub's pinned C/C++ CodeQL security analysis. |
| `.github/workflows/dependency-review.yml` | Rejects newly introduced high-severity vulnerable dependencies on pull requests. |
| `CMakeLists.txt` | Defines compiler policy, libraries, applications, tests, analysis modes, install rules, and packages. |
| `CMakePresets.json` | Provides repeatable developer, release, sanitizer, coverage, fuzz, and SDL configurations. |
| `Dockerfile` | Builds and runs the headless evidence demo as an unprivileged user. |
| `LICENSE` | Applies the MIT license to the repository. |
| `README.md` | Introduces the engine, demonstrates use, reports evidence, and routes readers to deeper documentation. |
| `CONTRIBUTING.md` | Documents development setup, quality gates, review expectations, and change workflow. |
| `SECURITY.md` | Defines supported versions, private vulnerability reporting, and defensive boundaries. |

## Public engine interface

| File | Responsibility |
|---|---|
| `include/forge2d/collision.hpp` | Declares collision items, normalized pairs, the spatial hash, and the quadratic reference oracle. |
| `include/forge2d/components.hpp` | Defines platform-independent transform, motion, sprite, and tick-input data. |
| `include/forge2d/entity.hpp` | Implements generation-safe entity handles, lifecycle management, and sparse-set component storage. |
| `include/forge2d/fixed_step.hpp` | Converts elapsed wall time into a bounded number of fixed simulation ticks. |
| `include/forge2d/math.hpp` | Provides overflow-safe integer vectors, AABBs, intersection, and clamping primitives. |
| `include/forge2d/pathfinding.hpp` | Declares a bounded tile grid and deterministic four-neighbor A* search. |
| `include/forge2d/profiler.hpp` | Implements a bounded rolling frame-time summary without unbounded telemetry growth. |
| `include/forge2d/renderer.hpp` | Declares the headless image buffer, PPM writer, pixel hash, and world renderer. |
| `include/forge2d/replay.hpp` | Defines the versioned replay model and strict parser/serializer interface. |
| `include/forge2d/scene.hpp` | Defines versioned scene data and strict parser/serializer interface. |
| `include/forge2d/sdl_renderer.hpp` | Declares the optional RAII-owned SDL3 presentation adapter. |
| `include/forge2d/world.hpp` | Exposes world lifecycle, simulation, snapshots, collision queries, state hashing, and replay execution. |

## Implementations and platform boundary

| File | Responsibility |
|---|---|
| `src/collision.cpp` | Builds bounded spatial cells, de-duplicates candidates, and exact-tests AABBs deterministically. |
| `src/pathfinding.cpp` | Implements deterministic A* with stable tie-breaking and bounded storage. |
| `src/replay.cpp` | Parses and serializes capped, version-one input replays. |
| `src/scene.cpp` | Parses, validates, and serializes capped, version-one scene descriptions. |
| `src/sdl_renderer.cpp` | Maps SDL3 events and accelerated rectangles onto the platform-neutral engine API. |
| `src/software_renderer.cpp` | Rasterizes clipped rectangles into deterministic RGBA memory and PPM evidence. |
| `src/world.cpp` | Implements entity lifetime, safe movement, bounds response, snapshots, collision input, and stable hashes. |
| `platform/arena.h` | Declares the small C17 caller-owned linear-arena boundary. |
| `platform/arena.c` | Implements checked alignment, capacity, overflow, zeroing, reset, and high-water accounting. |

## Applications, assets, tests, and tools

| File | Responsibility |
|---|---|
| `apps/forge2d_arena.cpp` | Runs the display-free autopilot scenario and emits replay, screenshot, and JSON evidence. |
| `apps/forge2d_arena_sdl.cpp` | Runs the optional interactive fixed-step SDL3 arena. |
| `assets/arena.scene` | Supplies the original, text-only demonstration arena and entities. |
| `benchmarks/benchmark.cpp` | Measures component lifecycle, collision broadphase, and fixed-tick throughput against regression budgets. |
| `tests/test_main.cpp` | Runs deterministic unit, randomized reference, negative, integration, and replay tests without a test dependency. |
| `fuzz/scene_parser_fuzz.cpp` | Exposes the scene parser to LLVM libFuzzer and asserts successful parse/serialize stability. |
| `fuzz/corpus/minimal.scene` | Seeds the fuzzer with a valid minimal version-one input. |
| `scripts/generate-code-index.mjs` | Generates exact class, function, and selected variable locations from authored code. |
| `scripts/validate-repository.mjs` | Enforces required docs, source headers, local links, and repository structure. |
| `scripts/verify.sh` | Runs the standard configure, build, test, benchmark, demo, and repository checks. |

## Engineering documentation and evidence

| File | Responsibility |
|---|---|
| `docs/adr/0001-headless-core-and-optional-sdl3.md` | Records why the dependency-free core and pinned optional SDL3 adapter were selected. |
| `docs/api.md` | Describes public contracts, lifetime rules, and short usage examples. |
| `docs/architecture.md` | Explains component boundaries, data flow, ownership, and extension points. |
| `docs/asset-format.md` | Specifies scene/replay grammars, validation limits, and compatibility rules. |
| `docs/code-index.md` | Generated declaration and important-variable line map for code review navigation. |
| `docs/complexity.md` | States Big-O time/space costs, locality trade-offs, and benchmark interpretation. |
| `docs/determinism.md` | Defines deterministic inputs, ordering, hashing, and the limits of the guarantee. |
| `docs/file-catalog.md` | Summarizes every committed file and its single responsibility. |
| `docs/gameplay.md` | Describes the arena scenario, controls, replay loop, and possible extensions. |
| `docs/operations.md` | Covers builds, runtime operation, troubleshooting, release, and recovery. |
| `docs/research.md` | Connects primary documentation to concrete engineering choices in this repository. |
| `docs/testing.md` | Maps risks to tests, CI environments, coverage, fuzzing, sanitizers, and performance gates. |
| `docs/reports/benchmark.md` | Captures the benchmark method, workload, budgets, and dated local results. |
| `docs/reports/test-summary.md` | Records test scope, dated local results, and limitations. |
| `docs/reports/validation.md` | Records compiler, SDL, repository, artifact, and pending hosted-CI validation. |
| `docs/reports/generated/.gitkeep` | Preserves the otherwise-empty directory used for generated CI reports. |
