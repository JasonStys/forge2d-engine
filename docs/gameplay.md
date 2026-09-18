# Arena sample

The sample is a deliberately small systems showcase rather than a content-heavy game. A player and
three moving drones share a bounded 10,000 × 10,000 simulation arena with four static obstacles.
All visuals are colored rectangles authored in the scene file.

## Headless demo

The headless executable builds a 20 × 20 navigation grid, places a wall with one opening, finds a
stable A* route, converts successive path segments into 360 `InputFrame` values, runs the world,
renders the final snapshot, and emits replay/performance identity evidence.

This path exercises the same engine contracts as the interactive sample without requiring display
hardware. It is the preferred recruiter/CI demonstration because output is repeatable.

## Interactive demo

The SDL3 executable loads the same scene, uses a bounded 60 Hz `FixedStepClock`, and maps arrow keys
to movement. Escape exits and Space populates the action field for future mechanics. Rendering uses
SDL's cross-platform renderer with primitive batching delegated to SDL.

## Suggested 90-second walkthrough

1. Run the headless executable and point out tick/entity/collision/state/pixel evidence.
2. Open the generated PPM and replay text.
3. Show `SparseSet<T>`, the spatial hash, and the brute-force property oracle.
4. Explain why the core is independent of SDL and how the SDL target is compiled separately.
5. Show sanitizer/fuzz/coverage/cross-platform GitHub Actions and the benchmark report.

## Extension ideas

Texture-atlas batching, animation state machines, deterministic collision response, object pools,
audio, and an ImGui debug overlay are valuable next steps only when accompanied by a measurable
sample need and new verification. They are not implied by the current version.
