# ADR-0001: Dependency-free headless core with optional SDL3 presentation

**Status:** Accepted
**Date:** 2026-09-18
**Deciders:** Repository owner

## Context

The engine must demonstrate real cross-platform presentation while keeping simulation, algorithms,
parsers, tests, sanitizers, and benchmarks runnable in headless CI. It should avoid forcing graphics
dependencies into downstream code and keep third-party code auditable and reproducible.

## Decision

Build a dependency-free C++20 static core plus a C17 arena. Provide two presentation adapters: a
deterministic software framebuffer/PPM writer compiled by default and an optional SDL3 renderer.
Fetch SDL 3.4.16 only when `FORGE2D_BUILD_SDL=ON`, using the official release archive and checked
SHA-256. Use SDL's renderer API for window/input/rectangle presentation rather than exposing SDL
types throughout the core.

## Options considered

### Option A: SDL3 throughout the engine

| Dimension | Assessment |
|---|---|
| Complexity | Medium |
| CI/display independence | Low |
| Rendering capability | High |
| Coupling | High |

**Pros:** fewer abstractions; direct access to mature cross-platform input/rendering.

**Cons:** core consumers/tests inherit SDL and display concerns; simulation boundaries blur.

### Option B: Dependency-free core plus optional SDL3 adapter

| Dimension | Assessment |
|---|---|
| Complexity | Medium |
| CI/display independence | High |
| Rendering capability | Medium |
| Coupling | Low |

**Pros:** fast headless tests, deterministic screenshots, small public API, optional real window.

**Cons:** two render paths; sample currently draws primitives rather than texture batches.

### Option C: Custom native backends

| Dimension | Assessment |
|---|---|
| Complexity | Very high |
| CI/display independence | Medium |
| Rendering capability | Potentially high |
| Maintenance | Very high |

**Pros:** maximum control and low-level graphics depth.

**Cons:** excessive scope across Direct3D/Metal/Vulkan/input/audio for a compact 2D portfolio engine.

## Trade-off analysis

Option B creates a small adapter cost but maximizes testability and keeps evidence focused on engine
algorithms. SDL's public stable release provides credible platform coverage without making a large
graphics abstraction the center of the project. The software path also gives byte-stable visual
evidence unavailable from driver-dependent pixels.

## Consequences

- Core builds need only a C++20/C17 toolchain.
- SDL downloads/build time occurs only in the dedicated configuration/job.
- The two renderers need contract-level consistency tests as visual scope grows.
- Advanced batching/textures can be added behind the adapter without changing simulation.
- SDL version/hash updates require an explicit reviewed source change.

## Action items

1. [x] Implement sorted immutable snapshots and software evidence renderer.
2. [x] Compile the pinned SDL3 adapter on Windows and Linux CI.
3. [x] Document controls, build modes, and limitations.
4. [ ] Add texture-atlas batching only after a measured sample requires it.
